/*
 * I/O functions for libusb
 * Copyright (C) 2007-2009 Daniel Drake <dsd@gentoo.org>
 * Copyright (c) 2001 Johannes Erdfelt <johannes@erdfelt.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

//#include <config.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef USBI_TIMERFD_AVAILABLE
#include <sys/timerfd.h>
#endif
#include "libusbi.h"
#include "pthread.h"

const struct usbi_os_backend * const usbi_backend = &linux_usbfs_backend;

int usbi_mutex_init_recursive(pthread_mutex_t *mutex, pthread_mutexattr_t *attr)
{
	int err;
	pthread_mutexattr_t stack_attr;
	if (!attr) {
		attr = &stack_attr;
		err = pthread_mutexattr_init(&stack_attr);
		if (err != 0)
			return err;
	}

	err = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_RECURSIVE);
	if (err != 0)
		goto finish;

	err = pthread_mutex_init(mutex, attr);

finish:
	if (attr == &stack_attr)
		pthread_mutexattr_destroy(&stack_attr);

	return err;
}


int usbi_io_init(struct libusb_context *ctx)
{
	int r;

	usbi_mutex_init(&ctx->flying_transfers_lock, NULL);
	usbi_mutex_init(&ctx->pollfds_lock, NULL);
	usbi_mutex_init(&ctx->pollfd_modify_lock, NULL);
	usbi_mutex_init_recursive(&ctx->events_lock, NULL); 
	//notice 20150528 jyc
	usbi_mutex_init(&ctx->event_waiters_lock, NULL);
	usbi_cond_init(&ctx->event_waiters_cond, NULL);
	list_init(&ctx->flying_transfers);
	list_init(&ctx->pollfds);

	/* FIXME should use an eventfd on kernels that support it */
	r = usbi_pipe(ctx->ctrl_pipe);
	if (r < 0) {
		r = LIBUSB_ERROR_OTHER;
		goto err;
	}

	r = usbi_add_pollfd(ctx, ctx->ctrl_pipe[0], POLLIN);
	if (r < 0)
		goto err_close_pipe;

#ifdef USBI_TIMERFD_AVAILABLE
	ctx->timerfd = timerfd_create(usbi_backend->get_timerfd_clockid(),
		TFD_NONBLOCK);
	if (ctx->timerfd >= 0) {
		usbi_dbg("using timerfd for timeouts");
		r = usbi_add_pollfd(ctx, ctx->timerfd, POLLIN);
		if (r < 0) {
			usbi_remove_pollfd(ctx, ctx->ctrl_pipe[0]);
			close(ctx->timerfd);
			goto err_close_pipe;
		}
	} else {
		usbi_dbg("timerfd not available (code %d error %d)", ctx->timerfd, errno);
		ctx->timerfd = -1;
	}
#endif

	return 0;

err_close_pipe:
	usbi_close(ctx->ctrl_pipe[0]);
	usbi_close(ctx->ctrl_pipe[1]);
err:
	usbi_mutex_destroy(&ctx->flying_transfers_lock);
	usbi_mutex_destroy(&ctx->pollfds_lock);
	usbi_mutex_destroy(&ctx->pollfd_modify_lock);
	usbi_mutex_destroy(&ctx->events_lock);
	usbi_mutex_destroy(&ctx->event_waiters_lock);
	usbi_cond_destroy(&ctx->event_waiters_cond);
	return r;
}

void usbi_io_exit(struct libusb_context *ctx)
{
	usbi_remove_pollfd(ctx, ctx->ctrl_pipe[0]);
	usbi_close(ctx->ctrl_pipe[0]);
	usbi_close(ctx->ctrl_pipe[1]);
#ifdef USBI_TIMERFD_AVAILABLE
	if (usbi_using_timerfd(ctx)) {
		usbi_remove_pollfd(ctx, ctx->timerfd);
		close(ctx->timerfd);
	}
#endif
	usbi_mutex_destroy(&ctx->flying_transfers_lock);
	usbi_mutex_destroy(&ctx->pollfds_lock);
	usbi_mutex_destroy(&ctx->pollfd_modify_lock);
	usbi_mutex_destroy(&ctx->events_lock);
	usbi_mutex_destroy(&ctx->event_waiters_lock);
	usbi_cond_destroy(&ctx->event_waiters_cond);
}

static int calculate_timeout(struct usbi_transfer *transfer)
{
	int r;
	struct timespec current_time;
	unsigned int timeout =
		USBI_TRANSFER_TO_LIBUSB_TRANSFER(transfer)->timeout;

	if (!timeout)
		return 0;

	r = usbi_backend->clock_gettime(USBI_CLOCK_MONOTONIC, &current_time);
	if (r < 0) {
		usbi_err(ITRANSFER_CTX(transfer),
			"failed to read monotonic clock, errno=%d", errno);
		return r;
	}

	current_time.tv_sec += timeout / 1000;
	current_time.tv_nsec += (timeout % 1000) * 1000000;

	if (current_time.tv_nsec > 1000000000) {
		current_time.tv_nsec -= 1000000000;
		current_time.tv_sec++;
	}

	TIMESPEC_TO_TIMEVAL(&transfer->timeout, &current_time);
	return 0;
}

/* add a transfer to the (timeout-sorted) active transfers list.
 * returns 1 if the transfer has a timeout and it is the timeout next to
 * expire */
static int add_to_flying_list(struct usbi_transfer *transfer)
{
	struct usbi_transfer *cur;
	struct timeval *timeout = &transfer->timeout;
	struct libusb_context *ctx = ITRANSFER_CTX(transfer);
	int r = 0;
	int first = 1;

	usbi_mutex_lock(&ctx->flying_transfers_lock);

	/* if we have no other flying transfers, start the list with this one */
	if (list_empty(&ctx->flying_transfers)) {
		list_add(&transfer->list, &ctx->flying_transfers);
		if (timerisset(timeout))
			r = 1;
		goto out;
	}

	/* if we have infinite timeout, append to end of list */
	if (!timerisset(timeout)) {
		list_add_tail(&transfer->list, &ctx->flying_transfers);
		goto out;
	}

	/* otherwise, find appropriate place in list */
	list_for_each_entry(cur, &ctx->flying_transfers, list, struct usbi_transfer) {
		/* find first timeout that occurs after the transfer in question */
		struct timeval *cur_tv = &cur->timeout;

		if (!timerisset(cur_tv) || (cur_tv->tv_sec > timeout->tv_sec) ||
				(cur_tv->tv_sec == timeout->tv_sec &&
					cur_tv->tv_usec > timeout->tv_usec)) {
			list_add_tail(&transfer->list, &cur->list);
			r = first;
			goto out;
		}
		first = 0;
	}

	/* otherwise we need to be inserted at the end */
	list_add_tail(&transfer->list, &ctx->flying_transfers);
out:
	usbi_mutex_unlock(&ctx->flying_transfers_lock);
	return r;
}

/** \ingroup asyncio
 * Allocate a libusb transfer with a specified number of isochronous packet
 * descriptors. The returned transfer is pre-initialized for you. When the new
 * transfer is no longer needed, it should be freed with
 * libusb_free_transfer().
 *
 * Transfers intended for non-isochronous endpoints (e.g. control, bulk,
 * interrupt) should specify an iso_packets count of zero.
 *
 * For transfers intended for isochronous endpoints, specify an appropriate
 * number of packet descriptors to be allocated as part of the transfer.
 * The returned transfer is not specially initialized for isochronous I/O;
 * you are still required to set the
 * \ref libusb_transfer::num_iso_packets "num_iso_packets" and
 * \ref libusb_transfer::type "type" fields accordingly.
 *
 * It is safe to allocate a transfer with some isochronous packets and then
 * use it on a non-isochronous endpoint. If you do this, ensure that at time
 * of submission, num_iso_packets is 0 and that type is set appropriately.
 *
 * \param iso_packets number of isochronous packet descriptors to allocate
 * \returns a newly allocated transfer, or NULL on error
 */
/*DEFAULT_VISIBILITY*/
struct libusb_transfer * libusb_alloc_transfer(
	int iso_packets)
{
	size_t os_alloc_size = usbi_backend->transfer_priv_size
		+ (usbi_backend->add_iso_packet_size * iso_packets);
	size_t alloc_size = sizeof(struct usbi_transfer)
		+ sizeof(struct libusb_transfer)
		+ (sizeof(struct libusb_iso_packet_descriptor) * iso_packets)
		+ os_alloc_size;
	struct usbi_transfer *itransfer = malloc(alloc_size);
	if (!itransfer)
		return NULL;

	memset(itransfer, 0, alloc_size);
	itransfer->num_iso_packets = iso_packets;
	usbi_mutex_init(&itransfer->lock, NULL);
	return USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
}

/** \ingroup asyncio
 * Free a transfer structure. This should be called for all transfers
 * allocated with libusb_alloc_transfer().
 *
 * If the \ref libusb_transfer_flags::LIBUSB_TRANSFER_FREE_BUFFER
 * "LIBUSB_TRANSFER_FREE_BUFFER" flag is set and the transfer buffer is
 * non-NULL, this function will also free the transfer buffer using the
 * standard system memory allocator (e.g. free()).
 *
 * It is legal to call this function with a NULL transfer. In this case,
 * the function will simply return safely.
 *
 * It is not legal to free an active transfer (one which has been submitted
 * and has not yet completed).
 *
 * \param transfer the transfer to free
 */
void API_EXPORTED libusb_free_transfer(struct libusb_transfer *transfer)
{
	struct usbi_transfer *itransfer;
	if (!transfer)
		return;

	if (transfer->flags & LIBUSB_TRANSFER_FREE_BUFFER && transfer->buffer)
		free(transfer->buffer);

	itransfer = LIBUSB_TRANSFER_TO_USBI_TRANSFER(transfer);
	usbi_mutex_destroy(&itransfer->lock);
	free(itransfer);
}

/** \ingroup asyncio
 * Submit a transfer. This function will fire off the USB transfer and then
 * return immediately.
 *
 * \param transfer the transfer to submit
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns LIBUSB_ERROR_BUSY if the transfer has already been submitted.
 * \returns LIBUSB_ERROR_NOT_SUPPORTED if the transfer flags are not supported
 * by the operating system.
 * \returns another LIBUSB_ERROR code on other failure
 */
int API_EXPORTED libusb_submit_transfer(struct libusb_transfer *transfer)
{
	struct libusb_context *ctx = TRANSFER_CTX(transfer);
	struct usbi_transfer *itransfer =
		LIBUSB_TRANSFER_TO_USBI_TRANSFER(transfer);
	int r;
	int first;

	usbi_mutex_lock(&itransfer->lock);
	itransfer->transferred = 0;
	itransfer->flags = 0;
	r = calculate_timeout(itransfer);
	if (r < 0) {
		r = LIBUSB_ERROR_OTHER;
		goto out;
	}

	first = add_to_flying_list(itransfer);
	r = usbi_backend->submit_transfer(itransfer);
	if (r) {
		usbi_mutex_lock(&ctx->flying_transfers_lock);
		list_del(&itransfer->list);
		usbi_mutex_unlock(&ctx->flying_transfers_lock);
	}
#ifdef USBI_TIMERFD_AVAILABLE
	else if (first && usbi_using_timerfd(ctx)) {
		/* if this transfer has the lowest timeout of all active transfers,
		 * rearm the timerfd with this transfer's timeout */
		const struct itimerspec it = { {0, 0},
			{ itransfer->timeout.tv_sec, itransfer->timeout.tv_usec * 1000 } };
		usbi_dbg("arm timerfd for timeout in %dms (first in line)", transfer->timeout);
		r = timerfd_settime(ctx->timerfd, TFD_TIMER_ABSTIME, &it, NULL);
		if (r < 0)
			r = LIBUSB_ERROR_OTHER;
	}
#else
	(void)first;
#endif

out:
	usbi_mutex_unlock(&itransfer->lock);
	return r;
}

/** \ingroup asyncio
 * Asynchronously cancel a previously submitted transfer.
 * This function returns immediately, but this does not indicate cancellation
 * is complete. Your callback function will be invoked at some later time
 * with a transfer status of
 * \ref libusb_transfer_status::LIBUSB_TRANSFER_CANCELLED
 * "LIBUSB_TRANSFER_CANCELLED."
 *
 * \param transfer the transfer to cancel
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the transfer is already complete or
 * cancelled.
 * \returns a LIBUSB_ERROR code on failure
 */
int API_EXPORTED libusb_cancel_transfer(struct libusb_transfer *transfer)
{
	struct usbi_transfer *itransfer =
		LIBUSB_TRANSFER_TO_USBI_TRANSFER(transfer);
	int r;

	usbi_dbg("");
	usbi_mutex_lock(&itransfer->lock);
	r = usbi_backend->cancel_transfer(itransfer);
	if (r < 0) {
		if (r != LIBUSB_ERROR_NOT_FOUND)
			usbi_err(TRANSFER_CTX(transfer),
				"cancel transfer failed error %d", r);
		else
			usbi_dbg("cancel transfer failed error %d", r);

		if (r == LIBUSB_ERROR_NO_DEVICE)
			itransfer->flags |= USBI_TRANSFER_DEVICE_DISAPPEARED;
	}

	itransfer->flags |= USBI_TRANSFER_CANCELLING;

	usbi_mutex_unlock(&itransfer->lock);
	return r;
}

#ifdef USBI_TIMERFD_AVAILABLE
static int disarm_timerfd(struct libusb_context *ctx)
{
	const struct itimerspec disarm_timer = { { 0, 0 }, { 0, 0 } };
	int r;

	usbi_dbg("");
	r = timerfd_settime(ctx->timerfd, 0, &disarm_timer, NULL);
	if (r < 0)
		return LIBUSB_ERROR_OTHER;
	else
		return 0;
}

/* iterates through the flying transfers, and rearms the timerfd based on the
 * next upcoming timeout.
 * must be called with flying_list locked.
 * returns 0 if there was no timeout to arm, 1 if the next timeout was armed,
 * or a LIBUSB_ERROR code on failure.
 */
static int arm_timerfd_for_next_timeout(struct libusb_context *ctx)
{
	struct usbi_transfer *transfer;

	list_for_each_entry(transfer, &ctx->flying_transfers, list, struct usbi_transfer) {
		struct timeval *cur_tv = &transfer->timeout;

		/* if we've reached transfers of infinite timeout, then we have no
		 * arming to do */
		if (!timerisset(cur_tv))
			return 0;

		/* act on first transfer that is not already cancelled */
		if (!(transfer->flags & USBI_TRANSFER_TIMED_OUT)) {
			int r;
			const struct itimerspec it = { {0, 0},
				{ cur_tv->tv_sec, cur_tv->tv_usec * 1000 } };
			usbi_dbg("next timeout originally %dms", USBI_TRANSFER_TO_LIBUSB_TRANSFER(transfer)->timeout);
			r = timerfd_settime(ctx->timerfd, TFD_TIMER_ABSTIME, &it, NULL);
			if (r < 0)
				return LIBUSB_ERROR_OTHER;
			return 1;
		}
	}

	return 0;
}
#Else
static int disarm_timerfd(struct libusb_context *ctx)
{
	(void)ctx;
	return 0;
}
static int arm_timerfd_for_next_timeout(struct libusb_context *ctx)
{
	(void)ctx;
	return 0;
}
#endif

/* Handle completion of a transfer (completion might be an error condition).
 * This will invoke the user-supplied callback function, which may end up
 * freeing the transfer. Therefore you cannot use the transfer structure
 * after calling this function, and you should free all backend-specific
 * data before calling it.
 * Do not call this function with the usbi_transfer lock held. User-specified
 * callback functions may attempt to directly resubmit the transfer, which
 * will attempt to take the lock. */
int usbi_handle_transfer_completion(struct usbi_transfer *itransfer,
	enum libusb_transfer_status status)
{
	struct libusb_transfer *transfer =
		USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	struct libusb_context *ctx = TRANSFER_CTX(transfer);
	uint8_t flags;
	int r = 0;

	/* FIXME: could be more intelligent with the timerfd here. we don't need
	 * to disarm the timerfd if there was no timer running, and we only need
	 * to rearm the timerfd if the transfer that expired was the one with
	 * the shortest timeout. */

	usbi_mutex_lock(&ctx->flying_transfers_lock);
	list_del(&itransfer->list);
	if (usbi_using_timerfd(ctx))
		r = arm_timerfd_for_next_timeout(ctx);
	usbi_mutex_unlock(&ctx->flying_transfers_lock);

	if (usbi_using_timerfd(ctx)) {
		if (r < 0)
			return r;
		r = disarm_timerfd(ctx);
		if (r < 0)
			return r;
	}

	if (status == LIBUSB_TRANSFER_COMPLETED
			&& transfer->flags & LIBUSB_TRANSFER_SHORT_NOT_OK) {
		int rqlen = transfer->length;
		if (transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL)
			rqlen -= LIBUSB_CONTROL_SETUP_SIZE;
		if (rqlen != itransfer->transferred) {
			usbi_dbg("interpreting short transfer as error");
			status = LIBUSB_TRANSFER_ERROR;
		}
	}

	flags = transfer->flags;
	transfer->status = status;
	transfer->actual_length = itransfer->transferred;
	usbi_dbg("transfer %p has callback %p", transfer, transfer->callback);
	if (transfer->callback)
		transfer->callback(transfer);
	/* transfer might have been freed by the above call, do not use from
	 * this point. */
	if (flags & LIBUSB_TRANSFER_FREE_TRANSFER)
		libusb_free_transfer(transfer);
	usbi_mutex_lock(&ctx->event_waiters_lock);
	usbi_cond_broadcast(&ctx->event_waiters_cond);
	usbi_mutex_unlock(&ctx->event_waiters_lock);
	return 0;
}

/* Similar to usbi_handle_transfer_completion() but exclusively for transfers
 * that were asynchronously cancelled. The same concerns w.r.t. freeing of
 * transfers exist here.
 * Do not call this function with the usbi_transfer lock held. User-specified
 * callback functions may attempt to directly resubmit the transfer, which
 * will attempt to take the lock. */
int usbi_handle_transfer_cancellation(struct usbi_transfer *transfer)
{
	/* if the URB was cancelled due to timeout, report timeout to the user */
	if (transfer->flags & USBI_TRANSFER_TIMED_OUT) {
		usbi_dbg("detected timeout cancellation");
		return usbi_handle_transfer_completion(transfer, LIBUSB_TRANSFER_TIMED_OUT);
	}

	/* otherwise its a normal async cancel */
	return usbi_handle_transfer_completion(transfer, LIBUSB_TRANSFER_CANCELLED);
}


/** \ingroup poll
 * Attempt to acquire the event handling lock. This lock is used to ensure that
 * only one thread is monitoring libusb event sources at any one time.
 *
 * You only need to use this lock if you are developing an application
 * which calls poll() or select() on libusb's file descriptors directly.
 * If you stick to libusb's event handling loop functions (e.g.
 * libusb_handle_events()) then you do not need to be concerned with this
 * locking.
 *
 * While holding this lock, you are trusted to actually be handling events.
 * If you are no longer handling events, you must call libusb_unlock_events()
 * as soon as possible.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \returns 0 if the lock was obtained successfully
 * \returns 1 if the lock was not obtained (i.e. another thread holds the lock)
 * \see \ref mtasync
 */
int API_EXPORTED libusb_try_lock_events(libusb_context *ctx)
{
	int r;
	USBI_GET_CONTEXT(ctx);

	/* is someone else waiting to modify poll fds? 
	 * if so, don't let this thread
	 * start event handling */
	usbi_mutex_lock(&ctx->pollfd_modify_lock);
	r = ctx->pollfd_modify;
	usbi_mutex_unlock(&ctx->pollfd_modify_lock);
	if (r) {
		usbi_dbg("someone else is modifying poll fds");
		return 1;
	}

	r = usbi_mutex_trylock(&ctx->events_lock); 
	//r=pthread_mutex_trylock(&ctx->events_lock); //modify 20150528 jyc
	if (r)
		return 1;  //delete 20150528 jyc 

	ctx->event_handler_active = 1;
	return 0;
}

/** \ingroup poll
 * Acquire the event handling lock, blocking until successful acquisition if
 * it is contended. This lock is used to ensure that only one thread is
 * monitoring libusb event sources at any one time.
 *
 * You only need to use this lock if you are developing an application
 * which calls poll() or select() on libusb's file descriptors directly.
 * If you stick to libusb's event handling loop functions (e.g.
 * libusb_handle_events()) then you do not need to be concerned with this
 * locking.
 *
 * While holding this lock, you are trusted to actually be handling events.
 * If you are no longer handling events, you must call libusb_unlock_events()
 * as soon as possible.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \see \ref mtasync
 */
void API_EXPORTED libusb_lock_events(libusb_context *ctx)
{
	USBI_GET_CONTEXT(ctx);
	usbi_mutex_lock(&ctx->events_lock);
	ctx->event_handler_active = 1;
}

/** \ingroup poll
 * Release the lock previously acquired with libusb_try_lock_events() or
 * libusb_lock_events(). Releasing this lock will wake up any threads blocked
 * on libusb_wait_for_event().
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \see \ref mtasync
 */
void API_EXPORTED libusb_unlock_events(libusb_context *ctx)
{
	USBI_GET_CONTEXT(ctx);
	ctx->event_handler_active = 0;
	usbi_mutex_unlock(&ctx->events_lock);

	/* FIXME: perhaps we should be a bit more efficient by not broadcasting
	 * the availability of the events lock when we are modifying pollfds
	 * (check ctx->pollfd_modify)? */
	usbi_mutex_lock(&ctx->event_waiters_lock);
	usbi_cond_broadcast(&ctx->event_waiters_cond);
	usbi_mutex_unlock(&ctx->event_waiters_lock);
}

/** \ingroup poll
 * Determine if it is still OK for this thread to be doing event handling.
 *
 * Sometimes, libusb needs to temporarily pause all event handlers, and this
 * is the function you should use before polling file descriptors to see if
 * this is the case.
 *
 * If this function instructs your thread to give up the events lock, you
 * should just continue the usual logic that is documented in \ref mtasync.
 * On the next iteration, your thread will fail to obtain the events lock,
 * and will hence become an event waiter.
 *
 * This function should be called while the events lock is held: you don't
 * need to worry about the results of this function if your thread is not
 * the current event handler.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \returns 1 if event handling can start or continue
 * \returns 0 if this thread must give up the events lock
 * \see \ref fullstory "Multi-threaded I/O: the full story"
 */
int API_EXPORTED libusb_event_handling_ok(libusb_context *ctx)
{
	int r;
	USBI_GET_CONTEXT(ctx);

	/* is someone else waiting to modify poll fds? if so, don't let this thread
	 * continue event handling */
	usbi_mutex_lock(&ctx->pollfd_modify_lock);
	r = ctx->pollfd_modify;
	usbi_mutex_unlock(&ctx->pollfd_modify_lock);
	if (r) {
		usbi_dbg("someone else is modifying poll fds");
		return 0;
	}

	return 1;
}


/** \ingroup poll
 * Determine if an active thread is handling events (i.e. if anyone is holding
 * the event handling lock).
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \returns 1 if a thread is handling events
 * \returns 0 if there are no threads currently handling events
 * \see \ref mtasync
 */
int API_EXPORTED libusb_event_handler_active(libusb_context *ctx)
{
	int r;
	USBI_GET_CONTEXT(ctx);

	/* is someone else waiting to modify poll fds? if so, don't let this thread
	 * start event handling -- indicate that event handling is happening */
	usbi_mutex_lock(&ctx->pollfd_modify_lock);
	r = ctx->pollfd_modify;
	usbi_mutex_unlock(&ctx->pollfd_modify_lock);
	if (r) {
		usbi_dbg("someone else is modifying poll fds");
		return 1;
	}

	return ctx->event_handler_active;
}

/** \ingroup poll
 * Acquire the event waiters lock. This lock is designed to be obtained under
 * the situation where you want to be aware when events are completed, but
 * some other thread is event handling so calling libusb_handle_events() is not
 * allowed.
 *
 * You then obtain this lock, re-check that another thread is still handling
 * events, then call libusb_wait_for_event().
 *
 * You only need to use this lock if you are developing an application
 * which calls poll() or select() on libusb's file descriptors directly,
 * <b>and</b> may potentially be handling events from 2 threads simultaenously.
 * If you stick to libusb's event handling loop functions (e.g.
 * libusb_handle_events()) then you do not need to be concerned with this
 * locking.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \see \ref mtasync
 */
void API_EXPORTED libusb_lock_event_waiters(libusb_context *ctx)
{
	USBI_GET_CONTEXT(ctx);
	usbi_mutex_lock(&ctx->event_waiters_lock);
}

/** \ingroup poll
 * Release the event waiters lock.
 * \param ctx the context to operate on, or NULL for the default context
 * \see \ref mtasync
 */
void API_EXPORTED libusb_unlock_event_waiters(libusb_context *ctx)
{
	USBI_GET_CONTEXT(ctx);
	usbi_mutex_unlock(&ctx->event_waiters_lock);
}

/** \ingroup poll
 * Wait for another thread to signal completion of an event. Must be called
 * with the event waiters lock held, see libusb_lock_event_waiters().
 *
 * This function will block until any of the following conditions are met:
 * -# The timeout expires
 * -# A transfer completes
 * -# A thread releases the event handling lock through libusb_unlock_events()
 *
 * Condition 1 is obvious. Condition 2 unblocks your thread <em>after</em>
 * the callback for the transfer has completed. Condition 3 is important
 * because it means that the thread that was previously handling events is no
 * longer doing so, so if any events are to complete, another thread needs to
 * step up and start event handling.
 *
 * This function releases the event waiters lock before putting your thread
 * to sleep, and reacquires the lock as it is being woken up.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param tv maximum timeout for this blocking function. A NULL value
 * indicates unlimited timeout.
 * \returns 0 after a transfer completes or another thread stops event handling
 * \returns 1 if the timeout expired
 * \see \ref mtasync
 */
int API_EXPORTED libusb_wait_for_event(libusb_context *ctx, struct timeval *tv)
{
	struct timespec timeout;
	int r;

	USBI_GET_CONTEXT(ctx);
	if (tv == NULL) {
		usbi_cond_wait(&ctx->event_waiters_cond, &ctx->event_waiters_lock);
		return 0;
	}

	r = usbi_backend->clock_gettime(USBI_CLOCK_REALTIME, &timeout);
	if (r < 0) {
		usbi_err(ctx, "failed to read realtime clock, error %d", errno);
		return LIBUSB_ERROR_OTHER;
	}

	timeout.tv_sec += tv->tv_sec;
	timeout.tv_nsec += tv->tv_usec * 1000;
	if (timeout.tv_nsec > 1000000000) {
		timeout.tv_nsec -= 1000000000;
		timeout.tv_sec++;
	}

	r = usbi_cond_timedwait(&ctx->event_waiters_cond,
		&ctx->event_waiters_lock, &timeout);
	return (r == ETIMEDOUT);
}

static void handle_timeout(struct usbi_transfer *itransfer)
{
	struct libusb_transfer *transfer =
		USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	int r;

	itransfer->flags |= USBI_TRANSFER_TIMED_OUT;
	r = libusb_cancel_transfer(transfer);
	if (r < 0)
		usbi_warn(TRANSFER_CTX(transfer),
			"async cancel failed %d errno=%d", r, errno);
}

static int handle_timeouts_locked(struct libusb_context *ctx)
{
	int r;
	struct timespec systime_ts;
	struct timeval systime;
	struct usbi_transfer *transfer;

	if (list_empty(&ctx->flying_transfers))
		return 0;

	/* get current time */
	r = usbi_backend->clock_gettime(USBI_CLOCK_MONOTONIC, &systime_ts);
	if (r < 0)
		return r;

	TIMESPEC_TO_TIMEVAL(&systime, &systime_ts);

	/* iterate through flying transfers list, finding all transfers that
	 * have expired timeouts */
	list_for_each_entry(transfer, &ctx->flying_transfers, list, struct usbi_transfer) {
		struct timeval *cur_tv = &transfer->timeout;

		/* if we've reached transfers of infinite timeout, we're all done */
		if (!timerisset(cur_tv))
			return 0;

		/* ignore timeouts we've already handled */
		if (transfer->flags & (USBI_TRANSFER_TIMED_OUT | USBI_TRANSFER_OS_HANDLES_TIMEOUT))
			continue;

		/* if transfer has non-expired timeout, nothing more to do */
		if ((cur_tv->tv_sec > systime.tv_sec) ||
				(cur_tv->tv_sec == systime.tv_sec &&
					cur_tv->tv_usec > systime.tv_usec))
			return 0;

		/* otherwise, we've got an expired timeout to handle */
		handle_timeout(transfer);
	}
	return 0;
}

static int handle_timeouts(struct libusb_context *ctx)
{
	int r;
	USBI_GET_CONTEXT(ctx);
	usbi_mutex_lock(&ctx->flying_transfers_lock);
	r = handle_timeouts_locked(ctx);
	usbi_mutex_unlock(&ctx->flying_transfers_lock);
	return r;
}

#ifdef USBI_TIMERFD_AVAILABLE
static int handle_timerfd_trigger(struct libusb_context *ctx)
{
	int r;

	r = disarm_timerfd(ctx);
	if (r < 0)
		return r;

	usbi_mutex_lock(&ctx->flying_transfers_lock);

	/* process the timeout that just happened */
	r = handle_timeouts_locked(ctx);
	if (r < 0)
		goto out;

	/* arm for next timeout*/
	r = arm_timerfd_for_next_timeout(ctx);

out:
	usbi_mutex_unlock(&ctx->flying_transfers_lock);
	return r;
}
#endif



/* do the actual event handling. assumes that no other thread is concurrently
 * doing the same thing. */
static int handle_events(struct libusb_context *ctx, struct timeval *tv)
{
	int r;
	struct usbi_pollfd *ipollfd;
	POLL_NFDS_TYPE nfds = 0;
	struct pollfd *fds;
	int i = -1;
	int timeout_ms;

	usbi_mutex_lock(&ctx->pollfds_lock);
	list_for_each_entry(ipollfd, &ctx->pollfds, list, struct usbi_pollfd)
		nfds++;

	// TODO: malloc when number of fd's changes, not on every poll 
	fds = malloc(sizeof(*fds) * nfds);
	if (!fds) {
		usbi_mutex_unlock(&ctx->pollfds_lock);
		return LIBUSB_ERROR_NO_MEM;
	}

	list_for_each_entry(ipollfd, &ctx->pollfds, list, struct usbi_pollfd) {
		struct libusb_pollfd *pollfd = &ipollfd->pollfd;
		int fd = pollfd->fd;
		i++;
		fds[i].fd = fd;
		fds[i].events = pollfd->events;
		fds[i].revents = 0;
	}
	usbi_mutex_unlock(&ctx->pollfds_lock);

	

	timeout_ms = (tv->tv_sec * 1000) + (tv->tv_usec / 1000);

	// round up to next millisecond 
	if (tv->tv_usec % 1000)
		timeout_ms++;

	usbi_dbg("poll() %d fds with timeout in %dms", nfds, timeout_ms);
	r = usbi_poll(fds, nfds, timeout_ms);
	usbi_dbg("poll() returned %d", r);
	if (r == 0) {
		free(fds);
		return handle_timeouts(ctx);
	} else if (r == -1 && errno == EINTR) {
		free(fds);
		return LIBUSB_ERROR_INTERRUPTED;
	} else if (r < 0) {
		free(fds);
		usbi_err(ctx, "poll failed %d err=%d\n", r, errno);
		return LIBUSB_ERROR_IO;
	}
	// fd[0] is always the ctrl pipe 
	if (fds[0].revents) {
		/* another thread wanted to interrupt event handling, and it succeeded!
		 * handle any other events that cropped up at the same time, and
		 * simply return */
		usbi_dbg("caught a fish on the control pipe");

		if (r == 1) {
			r = 0;
			goto handled;
		} else {
			/* prevent OS backend from trying to handle events on ctrl pipe */
			fds[0].revents = 0;
			r--;
		}
	}
#ifdef USBI_TIMERFD_AVAILABLE
	/* on timerfd configurations, fds[1] is the timerfd */
	if (usbi_using_timerfd(ctx) && fds[1].revents) {
		/* timerfd indicates that a timeout has expired */
		int ret;
		usbi_dbg("timerfd triggered");

		ret = handle_timerfd_trigger(ctx);
		if (ret < 0) {
			/* return error code */
			r = ret;
			goto handled;
		} else if (r == 1) {
			/* no more active file descriptors, nothing more to do */
			r = 0;
			goto handled;
		} else {
			/* more events pending...
			 * prevent OS backend from trying to handle events on timerfd */
			fds[1].revents = 0;
			r--;
		}
	}
#endif
	r = usbi_backend->handle_events(ctx, fds, nfds, r);
	if (r)
		usbi_err(ctx, "backend handle_events failed with error %d", r);

handled:
	free(fds);
	return r;
}

/* returns the smallest of:
 *  1. timeout of next URB
 *  2. user-supplied timeout
 * returns 1 if there is an already-expired timeout, otherwise returns 0
 * and populates out
 */
static int get_next_timeout(libusb_context *ctx, struct timeval *tv,
	struct timeval *out)
{
	struct timeval timeout;
	int r = libusb_get_next_timeout(ctx, &timeout);
	if (r) {
		/* timeout already expired? */
		if (!timerisset(&timeout))
			return 1;

		/* choose the smallest of next URB timeout or user specified timeout */
		if (timercmp(&timeout, tv, <))
			*out = timeout;
		else
			*out = *tv;
	} else {
		*out = *tv;
	}
	return 0;
}

/** \ingroup poll
 * Handle any pending events.
 *
 * libusb determines "pending events" by checking if any timeouts have expired
 * and by checking the set of file descriptors for activity.
 *
 * If a zero timeval is passed, this function will handle any already-pending
 * events and then immediately return in non-blocking style.
 *
 * If a non-zero timeval is passed and no events are currenlibusb_try_lock_eventstly pending, this
 * function will block waiting for events to handle up until the specified
 * timeout. If an event arrives or a signal is raised, this function will
 * return early.
 *
 * If the parameter completed is not NULL then <em>after obtaining the event
 * handling lock</em> this function will return immediately if the integer
 * pointed to is not 0. This allows for race free waiting for the completion
 * of a specific transfer.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param tv the maximum time to block waiting for events, or an all zero
 * timeval struct for non-blocking mode
 * \param completed pointer to completion integer to check, or NULL
 * \returns 0 on success, or a LIBUSB_ERROR code on failure
 * \see \ref mtasynclibusb_try_lock_events
 */
int API_EXPORTED libusb_handle_events_timeout_completed(libusb_context *ctx,
	struct timeval *tv, int *completed)
{
	int r;
	struct timeval poll_timeout;

	USBI_GET_CONTEXT(ctx);
	r = get_next_timeout(ctx, tv, &poll_timeout);
	if (r) {
		/* timeout already expired */
		return handle_timeouts(ctx);
	}
	
retry:
	r = libusb_try_lock_events(ctx);
	if (r == 0) {
		if (completed == NULL || !*completed) {
			// we obtained the event lock: 
			// do our own event handling 
			usbi_dbg("doing our own event handling");
			r = handle_events(ctx, &poll_timeout);
		}
		libusb_unlock_events(ctx);
		return r;
	}
	/* another thread is doing event handling. wait for thread events that
	 * notify event completion. */
	libusb_lock_event_waiters(ctx);
	if (completed && *completed)
		goto already_done;
	if (!libusb_event_handler_active(ctx)) {
		/* we hit a race: whoever was event handling earlier finished in the
		 * time it took us to reach this point. try the cycle again. */
		libusb_unlock_event_waiters(ctx);
		usbi_dbg("event handler was active but went away, retrying");
		goto retry;
	}
	usbi_dbg("another thread is doing event handling");
	r = libusb_wait_for_event(ctx, &poll_timeout);
already_done:
	libusb_unlock_event_waiters(ctx);
	if (r < 0)
		return r;
	else if (r == 1)
		return handle_timeouts(ctx);
	else
		return 0;
}

/** \ingroup poll
 * Handle any pending events
 *
 * Like libusb_handle_events_timeout_completed(), but without the completed
 * parameter, calling this function is equivalent to calling
 * libusb_handle_events_timeout_completed() with a NULL completed parameter.
 *
 * This function is kept primarily for backwards compatibility.
 * All new code should call libusb_handle_events_completed() or
 * libusb_handle_events_timeout_completed() to avoid race conditions.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param tv the maximum time to block waiting for events, or an all zero
 * timeval struct for non-blocking mode
 * \returns 0 on success, or a LIBUSB_ERROR code on failure
 */
int API_EXPORTED libusb_handle_events_timeout(libusb_context *ctx,
	struct timeval *tv)
{
	return libusb_handle_events_timeout_completed(ctx, tv, NULL);
}

/** \ingroup poll
 * Handle any pending events in blocking mode. There is currently a timeout
 * hardcoded at 60 seconds but we plan to make it unlimited in future. For
 * finer control over whether this function is blocking or non-blocking, or
 * for control over the timeout, use libusb_handle_events_timeout_completed()
 * instead.
 *
 * This function is kept primarily for backwards compatibility.
 * All new code should call libusb_handle_events_completed() or
 * libusb_handle_events_timeout_completed() to avoid race conditions.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \returns 0 on success, or a LIBUSB_ERROR code on failure
 */
int API_EXPORTED libusb_handle_events(libusb_context *ctx)
{
	struct timeval tv;
	tv.tv_sec = 60;
	tv.tv_usec = 0;
	return libusb_handle_events_timeout_completed(ctx, &tv, NULL);
}

/** \ingroup poll
 * Handle any pending events in blocking mode.
 *
 * Like libusb_handle_events(), with the addition of a completed parameter
 * to allow for race free waiting for the completion of a specific transfer.
 *
 * See libusb_handle_events_timeout_completed() for details on the completed
 * parameter.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param completed pointer to completion integer to check, or NULL
 * \returns 0 on success, or a LIBUSB_ERROR code on failure
 * \see \ref mtasync
 */
int API_EXPORTED libusb_handle_events_completed(libusb_context *ctx,
	int *completed)
{
	struct timeval tv;
	tv.tv_sec = 60;
	tv.tv_usec = 0;
	return libusb_handle_events_timeout_completed(ctx, &tv, completed);
}

/** \ingroup poll
 * Handle any pending events by polling file descriptors, without checking if
 * any other threads are already doing so. Must be called with the event lock
 * held, see libusb_lock_events().
 *
 * This function is designed to be called under the situation where you have
 * taken the event lock and are calling poll()/select() directly on libusb's
 * file descriptors (as opposed to using libusb_handle_events() or similar).
 * You detect events on libusb's descriptors, so you then call this function
 * with a zero timeout value (while still holding the event lock).
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param tv the maximum time to block waiting for events, or zero for
 * non-blocking mode
 * \returns 0 on success, or a LIBUSB_ERROR code on failure
 * \see \ref mtasync
 */
int API_EXPORTED libusb_handle_events_locked(libusb_context *ctx,
	struct timeval *tv)
{
	int r;
	struct timeval poll_timeout;

	USBI_GET_CONTEXT(ctx);
	r = get_next_timeout(ctx, tv, &poll_timeout);
	if (r) {
		/* timeout already expired */
		return handle_timeouts(ctx);
	}

	return handle_events(ctx, &poll_timeout);
}

/** \ingroup poll
 * Determines whether your application must apply special timing considerations
 * when monitoring libusb's file descriptors.
 *
 * This function is only useful for applications which retrieve and poll
 * libusb's file descriptors in their own main loop (\ref pollmain).
 *
 * Ordinarily, libusb's event handler needs to be called into at specific
 * moments in time (in addition to times when there is activity on the file
 * descriptor set). The usual approach is to use libusb_get_next_timeout()
 * to learn about when the next timeout occurs, and to adjust your
 * poll()/select() timeout accordingly so that you can make a call into the
 * library at that time.
 *
 * Some platforms supported by libusb do not come with this baggage - any
 * events relevant to timing will be represented by activity on the file
 * descriptor set, and libusb_get_next_timeout() will always return 0.
 * This function allows you to detect whether you are running on such a
 * platform.
 *
 * Since v1.0.5.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \returns 0 if you must call into libusb at times determined by
 * libusb_get_next_timeout(), or 1 if all timeout events are handled internally
 * or through regular activity on the file descriptors.
 * \see \ref pollmain "Polling libusb file descriptors for event handling"
 */
int API_EXPORTED libusb_pollfds_handle_timeouts(libusb_context *ctx)
{
#if defined(USBI_TIMERFD_AVAILABLE)
	USBI_GET_CONTEXT(ctx);
	return usbi_using_timerfd(ctx);
#else
	(void)ctx;
	return 0;
#endif
}

/** \ingroup poll
 * Determine the next internal timeout that libusb needs to handle. You only
 * need to use this function if you are calling poll() or select() or similar
 * on libusb's file descriptors yourself - you do not need to use it if you
 * are calling libusb_handle_events() or a variant directly.
 *
 * You should call this function in your main loop in order to determine how
 * long to wait for select() or poll() to return results. libusb needs to be
 * called into at this timeout, so you should use it as an upper bound on
 * your select() or poll() call.
 *
 * When the timeout has expired, call into libusb_handle_events_timeout()
 * (perhaps in non-blocking mode) so that libusb can handle the timeout.
 *
 * This function may return 1 (success) and an all-zero timeval. If this is
 * the case, it indicates that libusb has a timeout that has already expired
 * so you should call libusb_handle_events_timeout() or similar immediately.
 * A return code of 0 indicates that there are no pending timeouts.
 *
 * On some platforms, this function will always returns 0 (no pending
 * timeouts). See \ref polltime.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param tv output location for a relative time against the current
 * clock in which libusb must be called into in order to process timeout events
 * \returns 0 if there are no pending timeouts, 1 if a timeout was returned,
 * or LIBUSB_ERROR_OTHER on failure
 */
int API_EXPORTED libusb_get_next_timeout(libusb_context *ctx,
	struct timeval *tv)
{
	struct usbi_transfer *transfer;
	struct timespec cur_ts;
	struct timeval cur_tv;
	struct timeval *next_timeout;
	int r;
	int found = 0;

	USBI_GET_CONTEXT(ctx);
	if (usbi_using_timerfd(ctx))
		return 0;

	usbi_mutex_lock(&ctx->flying_transfers_lock);
	if (list_empty(&ctx->flying_transfers)) {
		usbi_mutex_unlock(&ctx->flying_transfers_lock);
		usbi_dbg("no URBs, no timeout!");
		return 0;
	}

	/* find next transfer which hasn't already been processed as timed out */
	list_for_each_entry(transfer, &ctx->flying_transfers, list, struct usbi_transfer) {
		if (transfer->flags & (USBI_TRANSFER_TIMED_OUT | USBI_TRANSFER_OS_HANDLES_TIMEOUT))
			continue;

		/* no timeout for this transfer? */
		if (!timerisset(&transfer->timeout))
			continue;

		found = 1;
		break;
	}
	usbi_mutex_unlock(&ctx->flying_transfers_lock);

	if (!found) {
		usbi_dbg("no URB with timeout or all handled by OS; no timeout!");
		return 0;
	}

	next_timeout = &transfer->timeout;

	r = usbi_backend->clock_gettime(USBI_CLOCK_MONOTONIC, &cur_ts);
	if (r < 0) {
		usbi_err(ctx, "failed to read monotonic clock, errno=%d", errno);
		return LIBUSB_ERROR_OTHER;
	}
	TIMESPEC_TO_TIMEVAL(&cur_tv, &cur_ts);

	if (!timercmp(&cur_tv, next_timeout, <)) {
		usbi_dbg("first timeout already expired");
		timerclear(tv);
	} else {
		timersub(next_timeout, &cur_tv, tv);
		usbi_dbg("next timeout in %d.%06ds", tv->tv_sec, tv->tv_usec);
	}

	return 1;
}

/** \ingroup poll
 * Register notification functions for file descriptor additions/removals.
 * These functions will be invoked for every new or removed file descriptor
 * that libusb uses as an event source.
 *
 * To remove notifiers, pass NULL values for the function pointers.
 *
 * Note that file descriptors may have been added even before you register
 * these notifiers (e.g. at libusb_init() time).
 *
 * Additionally, note that the removal notifier may be called during
 * libusb_exit() (e.g. when it is closing file descriptors that were opened
 * and added to the poll set at libusb_init() time). If you don't want this,
 * remove the notifiers immediately before calling libusb_exit().
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param added_cb pointer to function for addition notifications
 * \param removed_cb pointer to function for removal notifications
 * \param user_data User data to be passed back to callbacks (useful for
 * passing context information)
 */
void API_EXPORTED libusb_set_pollfd_notifiers(libusb_context *ctx,
	libusb_pollfd_added_cb added_cb, libusb_pollfd_removed_cb removed_cb,
	void *user_data)
{
	USBI_GET_CONTEXT(ctx);
	ctx->fd_added_cb = added_cb;
	ctx->fd_removed_cb = removed_cb;
	ctx->fd_cb_user_data = user_data;
}

/* Add a file descriptor to the list of file descriptors to be monitored.
 * events should be specified as a bitmask of events passed to poll(), e.g.
 * POLLIN and/or POLLOUT. */
int usbi_add_pollfd(struct libusb_context *ctx, int fd, short events)
{
	struct usbi_pollfd *ipollfd = malloc(sizeof(*ipollfd));
	if (!ipollfd)
		return LIBUSB_ERROR_NO_MEM;

	usbi_dbg("add fd %d events %d", fd, events);
	ipollfd->pollfd.fd = fd;
	ipollfd->pollfd.events = events;
	usbi_mutex_lock(&ctx->pollfds_lock);
	list_add_tail(&ipollfd->list, &ctx->pollfds);
	usbi_mutex_unlock(&ctx->pollfds_lock);

	if (ctx->fd_added_cb)
		ctx->fd_added_cb(fd, events, ctx->fd_cb_user_data);
	return 0;
}

/* Remove a file descriptor from the list of file descriptors to be polled. */
void usbi_remove_pollfd(struct libusb_context *ctx, int fd)
{
	struct usbi_pollfd *ipollfd;
	int found = 0;

	usbi_dbg("remove fd %d", fd);
	usbi_mutex_lock(&ctx->pollfds_lock);
	list_for_each_entry(ipollfd, &ctx->pollfds, list, struct usbi_pollfd)
		if (ipollfd->pollfd.fd == fd) {
			found = 1;
			break;
		}

	if (!found) {
		usbi_dbg("couldn't find fd %d to remove", fd);
		usbi_mutex_unlock(&ctx->pollfds_lock);
		return;
	}

	list_del(&ipollfd->list);
	usbi_mutex_unlock(&ctx->pollfds_lock);
	free(ipollfd);
	if (ctx->fd_removed_cb)
		ctx->fd_removed_cb(fd, ctx->fd_cb_user_data);
}

/** \ingroup poll
 * Retrieve a list of file descriptors that should be polled by your main loop
 * as libusb event sources.
 *
 * The returned list is NULL-terminated and should be freed with free() when
 * done. The actual list contents must not be touched.
 *
 * As file descriptors are a Unix-specific concept, this function is not
 * available on Windows and will always return NULL.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \returns a NULL-terminated list of libusb_pollfd structures
 * \returns NULL on error
 * \returns NULL on platforms where the functionality is not available
 */
/*DEFAULT_VISIBILITY*/
const struct libusb_pollfd ** /*LIBUSB_CALL*/ libusb_get_pollfds(
	libusb_context *ctx)
{
#ifndef OS_WINDOWS
	struct libusb_pollfd **ret = NULL;
	struct usbi_pollfd *ipollfd;
	size_t i = 0;
	size_t cnt = 0;
	USBI_GET_CONTEXT(ctx);

	usbi_mutex_lock(&ctx->pollfds_lock);
	list_for_each_entry(ipollfd, &ctx->pollfds, list, struct usbi_pollfd)
		cnt++;

	ret = calloc(cnt + 1, sizeof(struct libusb_pollfd *));
	if (!ret)
		goto out;

	list_for_each_entry(ipollfd, &ctx->pollfds, list, struct usbi_pollfd)
		ret[i++] = (struct libusb_pollfd *) ipollfd;
	ret[cnt] = NULL;

out:
	usbi_mutex_unlock(&ctx->pollfds_lock);
	return (const struct libusb_pollfd **) ret;
#else
	usbi_err(ctx, "external polling of libusb's internal descriptors "\
		"is not yet supported on Windows platforms");
	return NULL;
#endif
}

/* Backends call this from handle_events to report disconnection of a device.
 * The transfers get cancelled appropriately.
 */
void usbi_handle_disconnect(struct libusb_device_handle *handle)
{
	struct usbi_transfer *cur;
	struct usbi_transfer *to_cancel;

	usbi_dbg("device %d.%d",
		handle->dev->bus_number, handle->dev->device_address);

	/* terminate all pending transfers with the LIBUSB_TRANSFER_NO_DEVICE
	 * status code.
	 *
	 * this is a bit tricky because:
	 * 1. we can't do transfer completion while holding flying_transfers_lock
	 * 2. the transfers list can change underneath us - if we were to build a
	 *    list of transfers to complete (while holding look), the situation
	 *    might be different by the time we come to free them
	 *
	 * so we resort to a loop-based approach as below
	 * FIXME: is this still potentially racy?
	 */

	while (1) {
		usbi_mutex_lock(&HANDLE_CTX(handle)->flying_transfers_lock);
		to_cancel = NULL;
		list_for_each_entry(cur, &HANDLE_CTX(handle)->flying_transfers, list, struct usbi_transfer)
			if (USBI_TRANSFER_TO_LIBUSB_TRANSFER(cur)->dev_handle == handle) {
				to_cancel = cur;
				break;
			}
		usbi_mutex_unlock(&HANDLE_CTX(handle)->flying_transfers_lock);

		if (!to_cancel)
			break;

		usbi_backend->clear_transfer_priv(to_cancel);
		usbi_handle_transfer_completion(to_cancel, LIBUSB_TRANSFER_NO_DEVICE);
	}

}
