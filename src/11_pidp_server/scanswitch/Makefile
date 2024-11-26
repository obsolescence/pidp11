SOURCES.c = \
	scansw.c	\
	pinctrl/gpiochip_bcm2712.c  pinctrl/gpiochip_rp1.c  pinctrl/util.c \
	pinctrl/gpiochip_bcm2835.c  pinctrl/gpiolib.c       pinctrl/pinctrl.c

scansw: $(SOURCES.c) 
	$(CC) -o scansw $(SOURCES.c) -I.

