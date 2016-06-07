#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>

#include <libusb.h>
#include <libusbi.h>
#include <version.h>

#define EP_INTR			(1 | LIBUSB_ENDPOINT_IN)
#define EP_DATA			(2 | LIBUSB_ENDPOINT_IN)
#define CTRL_IN			(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT		(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)
#define USB_RQ			0x04
#define INTR_LENGTH		64

//const struct usbi_os_backend * const usbi_backend = &linux_usbfs_backend;

enum {
	MODE_INIT = 0x00,
	MODE_AWAIT_FINGER_ON = 0x10,
	MODE_AWAIT_FINGER_OFF = 0x12,
	MODE_CAPTURE = 0x20,
	MODE_SHUT_UP = 0x30,
	MODE_READY = 0x80,
};

static int next_state(void);

enum {
	STATE_AWAIT_MODE_CHANGE_AWAIT_FINGER_ON = 1,
	STATE_AWAIT_IRQ_FINGER_DETECTED,
	STATE_AWAIT_MODE_CHANGE_CAPTURE,
	STATE_AWAIT_IMAGE,
	STATE_AWAIT_MODE_CHANGE_AWAIT_FINGER_OFF,
	STATE_AWAIT_IRQ_FINGER_REMOVED,
};

static int state = 0;
static struct libusb_device_handle *devh = NULL;
static unsigned char imgbuf[0x1b340];
static unsigned char irqbuf[INTR_LENGTH];
static struct libusb_transfer *img_transfer = NULL;
static struct libusb_transfer *irq_transfer = NULL;
static int img_idx = 0;
static int do_exit = 0;

const struct libusb_version libusb_version_internal = {
	LIBUSB_MAJOR, LIBUSB_MINOR, LIBUSB_MICRO, LIBUSB_NANO, LIBUSB_RC,
	LIBUSB_DESCRIBE
};

struct libusb_context *usbi_default_context = NULL;
static int default_context_refcnt = 0;
static usbi_mutex_static_t default_context_lock = USBI_MUTEX_INITIALIZER;

#define DISCOVERED_DEVICES_SIZE_STEP 8

/*descriptor.c */

int usbi_parse_descriptor(unsigned char *source, const char *descriptor,
	void *dest, int host_endian)
{
	unsigned char *sp = source, *dp = dest;
	uint16_t w;
	const char *cp;

	for (cp = descriptor; *cp; cp++) {
		switch (*cp) {
			case 'b':	/* 8-bit byte */
				*dp++ = *sp++;
				break;
			case 'w':	/* 16-bit word, convert from little endian to CPU */
				dp += ((uintptr_t)dp & 1);	/* Align to word boundary */

				if (host_endian) {
					memcpy(dp, sp, 2);
				} else {
					w = (sp[1] << 8) | sp[0];
					*((uint16_t *)dp) = w;
				}
				sp += 2;
				dp += 2;
				break;
		}
	}

	return (int) (sp - source);
}

int usbi_get_config_index_by_value(struct libusb_device *dev,
	uint8_t bConfigurationValue, int *idx)
{
	uint8_t i;

	usbi_dbg("value %d", bConfigurationValue);
	for (i = 0; i < dev->num_configurations; i++) {
		unsigned char tmp[6];
		int host_endian;
		int r = usbi_backend->get_config_descriptor(dev, i, tmp, sizeof(tmp),
			&host_endian);
		if (r < 0)
			return r;
		if (tmp[5] == bConfigurationValue) {
			*idx = i;
			return 0;
		}
	}

	*idx = -1;
	return 0;
}

int API_EXPORTED libusb_get_device_descriptor(libusb_device *dev,
	struct libusb_device_descriptor *desc)
{
	unsigned char raw_desc[DEVICE_DESC_LENGTH];
	int host_endian = 0;
	int r;

	usbi_dbg("");
	r = usbi_backend->get_device_descriptor(dev, raw_desc, &host_endian);
	if (r < 0)
		return r;

	memcpy((unsigned char *) desc, raw_desc, sizeof(raw_desc));
	if (!host_endian) {
		desc->bcdUSB = libusb_le16_to_cpu(desc->bcdUSB);
		desc->idVendor = libusb_le16_to_cpu(desc->idVendor);
		desc->idProduct = libusb_le16_to_cpu(desc->idProduct);
		desc->bcdDevice = libusb_le16_to_cpu(desc->bcdDevice);
	}
	return 0;
}

/*core.c*/

/** \ingroup dev
 * Increment the reference count of a device.
 * \param dev the device to reference
 * \returns the same device
 */
//DEFAULT_VISIBILITY
//libusb_device * LIBUSB_CALL libusb_ref_device(libusb_device *dev)
libusb_device * libusb_ref_device(libusb_device *dev)
{
	usbi_mutex_lock(&dev->lock);
	dev->refcnt++;
	usbi_mutex_unlock(&dev->lock);
	return dev;
}

/** \ingroup dev
 * Decrement the reference count of a device. If the decrement operation
 * causes the reference count to reach zero, the device shall be destroyed.
 * \param dev the device to unreference
 */
void API_EXPORTED libusb_unref_device(libusb_device *dev)
{
	int refcnt;

	if (!dev)
		return;

	usbi_mutex_lock(&dev->lock);
	refcnt = --dev->refcnt;
	usbi_mutex_unlock(&dev->lock);

	if (refcnt == 0) {
		usbi_dbg("destroy device %d.%d", dev->bus_number, dev->device_address);

		if (usbi_backend->destroy_device)
			usbi_backend->destroy_device(dev);

		usbi_mutex_lock(&dev->ctx->usb_devs_lock);
		list_del(&dev->list);
		usbi_mutex_unlock(&dev->ctx->usb_devs_lock);

		usbi_mutex_destroy(&dev->lock);
		free(dev);
	}
}

/* append a device to the discovered devices collection. may realloc itself,
 * returning new discdevs. returns NULL on realloc failure. */
struct discovered_devs *discovered_devs_append(
	struct discovered_devs *discdevs, struct libusb_device *dev)
{
	size_t len = discdevs->len;
	size_t capacity;

	/* if there is space, just append the device */
	if (len < discdevs->capacity) {
		discdevs->devices[len] = libusb_ref_device(dev);
		discdevs->len++;
		return discdevs;
	}

	/* exceeded capacity, need to grow */
	usbi_dbg("need to increase capacity");
	capacity = discdevs->capacity + DISCOVERED_DEVICES_SIZE_STEP;
	discdevs = realloc(discdevs,
		sizeof(*discdevs) + (sizeof(void *) * capacity));
	if (discdevs) {
		discdevs->capacity = capacity;
		discdevs->devices[len] = libusb_ref_device(dev);
		discdevs->len++;
	}

	return discdevs;
}

/* Perform some final sanity checks on a newly discovered device. If this
 * function fails (negative return code), the device should not be added
 * to the discovered device list. */
int usbi_sanitize_device(struct libusb_device *dev)
{
	int r;
	unsigned char raw_desc[DEVICE_DESC_LENGTH];
	uint8_t num_configurations;
	int host_endian;

	r = usbi_backend->get_device_descriptor(dev, raw_desc, &host_endian);
	if (r < 0)
		return r;

	num_configurations = raw_desc[DEVICE_DESC_LENGTH - 1];
	if (num_configurations > USB_MAXCONFIG) {
		usbi_err(DEVICE_CTX(dev), "too many configurations");
		return LIBUSB_ERROR_IO;
	} else if (0 == num_configurations)
		usbi_dbg("zero configurations, maybe an unauthorized device");

	dev->num_configurations = num_configurations;
	return 0;
}

/* Allocate a new device with a specific session ID. The returned device has
 * a reference count of 1. */
struct libusb_device *usbi_alloc_device(struct libusb_context *ctx,
	unsigned long session_id)
{
	size_t priv_size = usbi_backend->device_priv_size;
	struct libusb_device *dev = calloc(1, sizeof(*dev) + priv_size);
	int r;

	if (!dev)
		return NULL;

	r = usbi_mutex_init(&dev->lock, NULL);
	if (r) {
		free(dev);
		return NULL;
	}

	dev->ctx = ctx;
	dev->refcnt = 1;
	dev->session_data = session_id;
	dev->speed = LIBUSB_SPEED_UNKNOWN;
	memset(&dev->os_priv, 0, priv_size);

	usbi_mutex_lock(&ctx->usb_devs_lock);
	list_add(&dev->list, &ctx->usb_devs);
	usbi_mutex_unlock(&ctx->usb_devs_lock);
	return dev;
}

/* Examine libusb's internal list of known devices, looking for one with
 * a specific session ID. Returns the matching device if it was found, and
 * NULL otherwise. */
struct libusb_device *usbi_get_device_by_session_id(struct libusb_context *ctx,
	unsigned long session_id)
{
	struct libusb_device *dev;
	struct libusb_device *ret = NULL;

	usbi_mutex_lock(&ctx->usb_devs_lock);
	list_for_each_entry(dev, &ctx->usb_devs, list, struct libusb_device)
		if (dev->session_data == session_id) {
			ret = dev;
			break;
		}
	usbi_mutex_unlock(&ctx->usb_devs_lock);

	return ret;
}


int API_EXPORTED libusb_init(libusb_context **context)
{
	char *dbg = getenv("LIBUSB_DEBUG");
	struct libusb_context *ctx;
	int r = 0;

	usbi_mutex_static_lock(&default_context_lock);
	if (!context && usbi_default_context) {
		usbi_dbg("reusing default context");
		default_context_refcnt++;
		usbi_mutex_static_unlock(&default_context_lock);
		return 0;
	}

	ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		r = LIBUSB_ERROR_NO_MEM;
		goto err_unlock;
	}
	memset(ctx, 0, sizeof(*ctx));

	if (dbg) {
		ctx->debug = atoi(dbg);
		if (ctx->debug)
			ctx->debug_fixed = 1;
	}

	usbi_dbg("libusb-%d.%d.%d%s%s%s",
	         libusb_version_internal.major,
	         libusb_version_internal.minor,
	         libusb_version_internal.micro,
	         libusb_version_internal.rc,
	         libusb_version_internal.describe[0] ? " git:" : "",
	         libusb_version_internal.describe);

	if (usbi_backend->init) {
		r = usbi_backend->init(ctx);
		if (r)
			goto err_free_ctx;
	}

	usbi_mutex_init(&ctx->usb_devs_lock, NULL);
	usbi_mutex_init(&ctx->open_devs_lock, NULL);
	list_init(&ctx->usb_devs);
	list_init(&ctx->open_devs);

	r = usbi_io_init(ctx);
	if (r < 0) {
		if (usbi_backend->exit)
			usbi_backend->exit();
		goto err_destroy_mutex;
	}

	if (context) {
		*context = ctx;
	} else if (!usbi_default_context) {
		usbi_dbg("created default context");
		usbi_default_context = ctx;
		default_context_refcnt++;
	}
	usbi_mutex_static_unlock(&default_context_lock);

	return 0;

err_destroy_mutex:
	usbi_mutex_destroy(&ctx->open_devs_lock);
	usbi_mutex_destroy(&ctx->usb_devs_lock);
err_free_ctx:
	free(ctx);
err_unlock:
	usbi_mutex_static_unlock(&default_context_lock);
	return r;
}

static struct discovered_devs *discovered_devs_alloc(void)
{
	struct discovered_devs *ret =
		malloc(sizeof(*ret) + (sizeof(void *) * DISCOVERED_DEVICES_SIZE_STEP));

	if (ret) {
		ret->len = 0;
		ret->capacity = DISCOVERED_DEVICES_SIZE_STEP;
	}
	return ret;
}

static void discovered_devs_free(struct discovered_devs *discdevs)
{
	size_t i;

	for (i = 0; i < discdevs->len; i++)
		libusb_unref_device(discdevs->devices[i]);

	free(discdevs);
}

ssize_t API_EXPORTED libusb_get_device_list(libusb_context *ctx,
	libusb_device ***list)
{
	struct discovered_devs *discdevs = discovered_devs_alloc();
	struct libusb_device **ret;
	int r = 0;
	ssize_t i, len;
	USBI_GET_CONTEXT(ctx);
	usbi_dbg("");

	if (!discdevs)
		return LIBUSB_ERROR_NO_MEM;

	r = usbi_backend->get_device_list(ctx, &discdevs);
	if (r < 0) {
		len = r;
		goto out;
	}

	/* convert discovered_devs into a list */
	len = discdevs->len;
	ret = malloc(sizeof(void *) * (len + 1));
	if (!ret) {
		len = LIBUSB_ERROR_NO_MEM;
		goto out;
	}

	ret[len] = NULL;
	for (i = 0; i < len; i++) {
		struct libusb_device *dev = discdevs->devices[i];
		ret[i] = libusb_ref_device(dev);
	}
	*list = ret;

out:
	discovered_devs_free(discdevs);
	return len;
}

void usbi_fd_notification(struct libusb_context *ctx)
{
	unsigned char dummy = 1;
	ssize_t r;

	if (ctx == NULL)
		return;

	/* record that we are messing with poll fds */
	usbi_mutex_lock(&ctx->pollfd_modify_lock);
	ctx->pollfd_modify++;
	usbi_mutex_unlock(&ctx->pollfd_modify_lock);

	/* write some data on control pipe to interrupt event handlers */
	r = usbi_write(ctx->ctrl_pipe[1], &dummy, sizeof(dummy));
	if (r <= 0) {
		usbi_warn(ctx, "internal signalling write failed");
		usbi_mutex_lock(&ctx->pollfd_modify_lock);
		ctx->pollfd_modify--;
		usbi_mutex_unlock(&ctx->pollfd_modify_lock);
		return;
	}

	/* take event handling lock */
	libusb_lock_events(ctx);

	/* read the dummy data */
	r = usbi_read(ctx->ctrl_pipe[0], &dummy, sizeof(dummy));
	if (r <= 0)
		usbi_warn(ctx, "internal signalling read failed");

	/* we're done with modifying poll fds */
	usbi_mutex_lock(&ctx->pollfd_modify_lock);
	ctx->pollfd_modify--;
	usbi_mutex_unlock(&ctx->pollfd_modify_lock);

	/* Release event handling lock and wake up event waiters */
	libusb_unlock_events(ctx);
}

int API_EXPORTED libusb_open(libusb_device *dev,
	libusb_device_handle **handle)
{
	struct libusb_context *ctx = DEVICE_CTX(dev);
	struct libusb_device_handle *_handle;
	size_t priv_size = usbi_backend->device_handle_priv_size;
	int r;
	usbi_dbg("open %d.%d", dev->bus_number, dev->device_address);

	_handle = malloc(sizeof(*_handle) + priv_size);
	if (!_handle)
		return LIBUSB_ERROR_NO_MEM;

	r = usbi_mutex_init(&_handle->lock, NULL);
	if (r) {
		free(_handle);
		return LIBUSB_ERROR_OTHER;
	}

	_handle->dev = libusb_ref_device(dev);
	_handle->claimed_interfaces = 0;
	memset(&_handle->os_priv, 0, priv_size);

	r = usbi_backend->open(_handle);
	if (r < 0) {
		usbi_dbg("open %d.%d returns %d", dev->bus_number, dev->device_address, r);
		libusb_unref_device(dev);
		usbi_mutex_destroy(&_handle->lock);
		free(_handle);
		return r;
	}

	usbi_mutex_lock(&ctx->open_devs_lock);
	list_add(&_handle->list, &ctx->open_devs);
	usbi_mutex_unlock(&ctx->open_devs_lock);
	*handle = _handle;

	/* At this point, we want to interrupt any existing event handlers so
	 * that they realise the addition of the new device's poll fd. One
	 * example when this is desirable is if the user is running a separate
	 * dedicated libusb events handling thread, which is running with a long
	 * or infinite timeout. We want to interrupt that iteration of the loop,
	 * so that it picks up the new fd, and then continues. */
	usbi_fd_notification(ctx);

	return 0;
}

void API_EXPORTED libusb_free_device_list(libusb_device **list,
	int unref_devices)
{
	if (!list)
		return;

	if (unref_devices) {
		int i = 0;
		struct libusb_device *dev;

		while ((dev = list[i++]) != NULL)
			libusb_unref_device(dev);
	}
	free(list);
}

//DEFAULT_VISIBILITY
libusb_device_handle * libusb_open_device_with_vid_pid(
	libusb_context *ctx, uint16_t vendor_id, uint16_t product_id)
{
	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *handle = NULL;
	size_t i = 0;
	int r;

	if (libusb_get_device_list(ctx, &devs) < 0)
		return NULL;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0)
			goto out;
		if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
			found = dev;
			break;
		}
	}

	if (found) {
		r = libusb_open(found, &handle);
		if (r < 0)
			handle = NULL;
	}

out:
	libusb_free_device_list(devs, 1);
	return handle;
}

int API_EXPORTED libusb_detach_kernel_driver(libusb_device_handle *dev,
	int interface_number)
{
	usbi_dbg("interface %d", interface_number);
	if (usbi_backend->detach_kernel_driver)
		return usbi_backend->detach_kernel_driver(dev, interface_number);
	else
		return LIBUSB_ERROR_NOT_SUPPORTED;
}

int API_EXPORTED libusb_kernel_driver_active(libusb_device_handle *dev,
	int interface_number)
{
	usbi_dbg("interface %d", interface_number);
	if (usbi_backend->kernel_driver_active)
		return usbi_backend->kernel_driver_active(dev, interface_number);
	else
		return LIBUSB_ERROR_NOT_SUPPORTED;
}

int API_EXPORTED libusb_claim_interface(libusb_device_handle *dev,
	int interface_number)
{
	int r = 0;

	usbi_dbg("interface %d", interface_number);
	if (interface_number >= USB_MAXINTERFACES)
		return LIBUSB_ERROR_INVALID_PARAM;

	usbi_mutex_lock(&dev->lock);
	if (dev->claimed_interfaces & (1 << interface_number))
		goto out;

	r = usbi_backend->claim_interface(dev, interface_number);
	if (r == 0)
		dev->claimed_interfaces |= 1 << interface_number;

out:
	usbi_mutex_unlock(&dev->lock);
	return r;
}

/*sync.c*/
static void /*LIBUSB_CALL*/ bulk_transfer_cb(struct libusb_transfer *transfer)
{
	int *completed = transfer->user_data;
	*completed = 1;
	usbi_dbg("actual_length=%d", transfer->actual_length);
	/* caller interprets results and frees transfer */
}

static int do_sync_bulk_transfer(struct libusb_device_handle *dev_handle,
	unsigned char endpoint, unsigned char *buffer, int length,
	int *transferred, unsigned int timeout, unsigned char type)
{
	struct libusb_transfer *transfer = libusb_alloc_transfer(0);
	int completed = 0;
	int r;

	if (!transfer)
		return LIBUSB_ERROR_NO_MEM;

	libusb_fill_bulk_transfer(transfer, dev_handle, endpoint, buffer, length,
		bulk_transfer_cb, &completed, timeout);
	transfer->type = type;

	r = libusb_submit_transfer(transfer);
	if (r < 0) {
		libusb_free_transfer(transfer);
		return r;
	}

	while (!completed) {
		r = libusb_handle_events_completed(HANDLE_CTX(dev_handle), &completed);
		if (r < 0) {
			if (r == LIBUSB_ERROR_INTERRUPTED)
				continue;
			libusb_cancel_transfer(transfer);
			while (!completed)
				if (libusb_handle_events_completed(HANDLE_CTX(dev_handle), &completed) < 0)
					break;
			libusb_free_transfer(transfer);
			return r;
		}
	}
	*transferred = transfer->actual_length;
	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		r = 0;
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		r = LIBUSB_ERROR_TIMEOUT;
		break;
	case LIBUSB_TRANSFER_STALL:
		r = LIBUSB_ERROR_PIPE;
		break;
	case LIBUSB_TRANSFER_OVERFLOW:
		r = LIBUSB_ERROR_OVERFLOW;
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		r = LIBUSB_ERROR_NO_DEVICE;
		break;
	default:
		usbi_warn(HANDLE_CTX(dev_handle),
			"unrecognised status code %d", transfer->status);
		r = LIBUSB_ERROR_OTHER;
	}

	libusb_free_transfer(transfer);
	return r;
}

int API_EXPORTED libusb_bulk_transfer(struct libusb_device_handle *dev_handle,
	unsigned char endpoint, unsigned char *data, int length, int *transferred,
	unsigned int timeout)
{
	return do_sync_bulk_transfer(dev_handle, endpoint, data, length,
		transferred, timeout, LIBUSB_TRANSFER_TYPE_BULK);
}
