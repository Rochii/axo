TOP=.

REPOS=$(TOP)/../repos
BUILD=$(TOP)/axoloti
DL=$(TOP)/dl

CHIBIOS=ChibiOS_2.6.9.zip
GCC=gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2
DFU=dfu-util-0.8.tar.gz
LIBUSB=libusb-1.0.19.tar.bz2

PATCHFILES = $(sort $(wildcard patches/*.patch ))

PATCH_CMD = \
	for f in $(PATCHFILES); do\
	    echo $$f ":"; \
	    patch -d $(BUILD) -p0 -b -z .original < $$f; \
	done

COPY_CMD = \
	cd files; tar -c -f - * | (cd ../$(BUILD) ; tar xfp -)

all:
	tar zxvf $(DL)/axoloti.tgz 
	$(PATCH_CMD)
	$(COPY_CMD)
	cp $(DL)/$(CHIBIOS) $(BUILD)/platform_linux/src
	cp $(DL)/$(GCC) $(BUILD)/platform_linux/src
	cp $(DL)/$(DFU) $(BUILD)/platform_linux/src
	cp $(DL)/$(LIBUSB) $(BUILD)/platform_linux/src
	cd $(BUILD)/platform_linux; ./build.sh

tarball:
	tar -C $(REPOS) --exclude=.git -zcvf $(DL)/axoloti.tgz axoloti

clean:
	-rm -rf $(BUILD)
