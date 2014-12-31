obj-m = hide_process.o

#EXTRA_CFLAGS := -I/usr/include

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(CURDIR) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(CURDIR) clean