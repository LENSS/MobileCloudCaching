# Kernel module caching daemon

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
PKG_NAME:=caching
PKG_RELEASE:=1.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/caching
	SUBMENU:=Other modules
	DEPENDS:=@TARGET_ar71xx　　
	TITLE:=Support Module for caching
	AUTOLOAD:=$(call AutoLoad,81,caching)
	FILES:=$(HOME)/backfire/build_dir/linux-ar71xx/caching/caching.ko
endef

define Build/Prepare
	mkdir -p $(HOME)/backfire/build_dir/linux-ar71xx/caching
	cp -fpr $(HOME)/backfire/package/caching/src/* $(HOME)/backfire/build_dir/linux-ar71xx/caching/
endef

define Build/Compile
	$(MAKE) -C $(HOME)/backfire/build_dir/linux-ar71xx/linux-2.6.32.27 \
	CC="$(TARGET_CC)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	ARCH="$(ARCH)" \
	SUBDIRS="$(HOME)/backfire/build_dir/linux-ar71xx/caching/" \
	EXTRA_CFLAGS="$(TARGET_CFLAGS)" \
	modules
endef

$(eval $(call KernelPackage,caching))






