RM = rm
#CC = linux-gcc
OBJS = linux_usbfs.o io.o usbfunc.o algorithm.o serial.o main.o
bin = gsbox
CFLAGS += -I.
LIBS = -levent
LIBS = -lusb-1.0
LIBS = -lpthread


#LIBDIR = -L/lib/
all : $(bin)
$(bin) : $(OBJS)
	$(CC) $(CFLAGS) -Wall  $(OBJS) $(LIBDIR) -o $(bin) $(LIBS)
clean :
	$(RM) -f $(OBJS)
	$(RM) -f $(bin)
