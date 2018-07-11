TOP=.

BRANCH = master
#BRANCH = experimental

REPOS=$(TOP)/repos
BUILD=$(TOP)/axoloti
DL=$(TOP)/dl

ifeq ($(BRANCH),master)
  CHIBIOS=ChibiOS_2.6.9.zip
  GCC=gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2
else ifeq ($(BRANCH),experimental)
  CHIBIOS=ChibiOS_18.2.0.zip
  GCC=gcc-arm-none-eabi-7-2017-q4-major-linux.tar.bz2
else

endif

DFU=dfu-util-0.8.tar.gz
LIBUSB=libusb-1.0.19.tar.bz2

PATCHFILES = $(sort $(wildcard patches/$(BRANCH)/*.patch ))

PATCH_CMD = \
	for f in $(PATCHFILES); do\
	    echo $$f ":"; \
	    patch -d $(BUILD) -p0 -b -z .original < $$f; \
	done

COPY_CMD = \
	cd files/$(BRANCH); tar -c -f - * | (cd ../$(BUILD) ; tar xfp -)

define update_repo
	if [ ! -d $(REPOS)/$(1)/.git ]; then \
		git -C $(REPOS) clone git@github.com:axoloti/$(1).git; \
	else \
		git -C $(REPOS)/$(1) pull; \
	fi 
endef

.PHONY: all repos tarball update clean

all:
	tar zxf $(DL)/axoloti_$(BRANCH).tgz 
	$(PATCH_CMD)
	$(COPY_CMD)
	-cp $(DL)/$(CHIBIOS) $(BUILD)/platform_linux/src
	-cp $(DL)/$(GCC) $(BUILD)/platform_linux/src
	-cp $(DL)/$(GCC) $(BUILD)/platform_linux
	-cp $(DL)/$(DFU) $(BUILD)/platform_linux/src
	-cp $(DL)/$(LIBUSB) $(BUILD)/platform_linux/src
	-cp axoloti.prefs $(BUILD)
	cd $(BUILD)/platform_linux; ./build.sh

repos:
	if [ ! -d $(REPOS) ]; then mkdir $(REPOS); fi
	$(call update_repo,axoloti)
	$(call update_repo,axoloti-contrib)
	$(call update_repo,axoloti-factory)

tarball:
	git -C $(REPOS)/axoloti checkout $(BRANCH); git pull
	tar -C $(REPOS) -zcf $(DL)/axoloti_$(BRANCH).tgz axoloti

update: repos tarball

clean:
	-rm -rf $(BUILD)
