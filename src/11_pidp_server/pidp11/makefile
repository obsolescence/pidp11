

# call with MAKE_TARGET_ARCH=X86 or =RPI


#
# CC Command
#

# own settings
OS_CCDEFS =
CC_DBG_FLAGS = -ggdb3 -O0
LDFLAGS = -pthread

	# all other cases: X86
	OBJDIR=x86

ifeq ($(MAKE_TARGET_ARCH),BEAGLEBONE)
	#### !!! when cross compiling !!!!
	# fuer beaglebone und angstrom stand alone toolchain
	CC=arm-angstrom-linux-gnueabi-gcc -mthumb -march=armv7-a -std=c99 -U__STRICT_ANSI__ $(CC_DBG_FLAGS) $(OS_CCDEFS) -I.
	OBJDIR=bin-beaglebone
	SSHTARGET=root@beaglebone
	LDFLAGS += -lrt
else
ifeq ($(MAKE_TARGET_ARCH),RPI)
	CC=gcc -std=c99 -U__STRICT_ANSI__ $(CC_DBG_FLAGS) $(OS_CCDEFS) -I.
	OBJDIR=bin-rpi
	SSHTARGET=pi@raspberrypi
	LDFLAGS += -lrt -ltirpc
endif

endif

# Reference also the standard blinkenbone server in BLINKENLIGHT_SERVER_DIR

CCDEFS= \
	-DBLINKENLIGHT_SERVER	\
	-I.	\
	-I$(BLINKENLIGHT_COMMON_DIR)	\
	-I$(BLINKENLIGHT_API_DIR)	\
	-I$(BLINKENLIGHT_API_DIR)/rpcgen_linux	\
	-I$(BLINKENLIGHT_SERVER_DIR)	\
	-I/usr/include/tirpc	\
	$(CC_DBG_FLAGS) $(OS_CCDEFS)


BLINKENLIGHT_COMMON_DIR=../../00_common
# sub dir for the API. is same level (= sibling) as server and client
BLINKENLIGHT_API_DIR=../../07.0_blinkenlight_api
# dir of standard server
BLINKENLIGHT_SERVER_DIR=../../07.1_blinkenlight_server
# *_svc, *_xdr and api.h sources must be created with rpcgen
BLINKENLIGHT_API_SOURCES.c = \
	$(BLINKENLIGHT_API_DIR)/blinkenlight_panels.c	\
	$(BLINKENLIGHT_API_DIR)/blinkenlight_api_server_procs.c \
	$(BLINKENLIGHT_API_DIR)/rpcgen_linux/rpc_blinkenlight_api_svc.c \
	$(BLINKENLIGHT_API_DIR)/rpcgen_linux/rpc_blinkenlight_api_xdr.c	\
	$(BLINKENLIGHT_API_DIR)/historybuffer.c	\
	$(BLINKENLIGHT_COMMON_DIR)/bitcalc.c	\
	$(BLINKENLIGHT_COMMON_DIR)/radix.c	\
	$(BLINKENLIGHT_COMMON_DIR)/mcout.c	\
	$(BLINKENLIGHT_COMMON_DIR)/errno2txt.c

BLINKENLIGHT_API_SOURCES.h = \
	$(BLINKENLIGHT_API_DIR)/rpcgen_linux/rpc_blinkenlight_api.h	\
	$(BLINKENLIGHT_API_DIR)/blinkenlight_api_server_procs.h \
	$(BLINKENLIGHT_API_DIR)/historybuffer.h	\
	$(BLINKENLIGHT_COMMON_DIR)/bitcalc.h	\
	$(BLINKENLIGHT_COMMON_DIR)/radix.h	\
	$(BLINKENLIGHT_COMMON_DIR)/mcout.h	\
	$(BLINKENLIGHT_COMMON_DIR)/errno2txt.h

$(BLINKENLIGHT_API_SOURCES.c) $(BLINKENLIGHT_API_SOURCES.h):
	# rpcgen
	make --directory=$(BLINKENLIGHT_API_DIR)/rpcgen_linux sources


#########################################################

SOURCES.h = \
	main.h	\
	gpio.h	\
	gpiopattern.h	\
	$(BLINKENLIGHT_SERVER_DIR)/print.h	\
	$(BLINKENLIGHT_API_DIR)/blinkenlight_panels.h	\
	pinctrl/gpiochip.h	pinctrl/gpiolib.h pinctrl/util.h

SOURCES.c = \
	main.c	\
	gpio.c	\
	gpiopattern.c	\
	$(BLINKENLIGHT_SERVER_DIR)/print.c	\
	$(BLINKENLIGHT_API_SOURCES.c)	\
	pinctrl/gpiochip_bcm2712.c  pinctrl/gpiochip_rp1.c  pinctrl/util.c \
	pinctrl/gpiochip_bcm2835.c  pinctrl/gpiolib.c       pinctrl/pinctrl.c



OBJECTS = $(SOURCES.c:%.c=%.o)
#
# Build everything
#
all:    pidp1170_blinkenlightd

clean:
	echo "CLEAN"
	rm -f core pidp1170_blinkenlightd $(OBJECTS) $(OBJDIR)/pidp1170_blinkenlightd
	make --directory=$(BLINKENLIGHT_API_DIR)/rpcgen_linux clean

install:
	echo "copy server to beaglebone/rpi ... call only for ARM server!"
ifneq ($(MAKE_TARGET_ARCH),RPI)
	echo --- set remote time
	ssh $(SSHTARGET) "date -u `date -u +%m%d%H%M%Y.%S`; echo `cat /etc/timezone` > /etc/timezone"
endif
	echo copy module ...
	scp $(OBJDIR)/pidp1170_blinkenlightd $(SSHTARGET):


pidp1170_blinkenlightd:	$(SOURCES.c) $(SOURCES.h)
	# printenv | sort >env.make
	${CC} $^ -o $@ $(CCDEFS) ${LDFLAGS}
	mkdir -p $(OBJDIR)
	mv $@ $(OBJDIR)
	# Verify: was it x86 or ARM?
	file $(OBJDIR)/$@

