# Kernel module kaodv

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
PKG_NAME:=kaodv
PKG_RELEASE:=1.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/kaodv
	SUBMENU:=Other modules
	DEPENDS:=@TARGET_ar71xx　　
	TITLE:=Support Module for AODV routing
	AUTOLOAD:=$(call AutoLoad,81,kaodv)
	FILES:=$(HOME)/backfire/build_dir/linux-ar71xx/kaodv/kaodv.ko
endef

define Build/Prepare
	mkdir -p $(HOME)/backfire/build_dir/linux-ar71xx/kaodv
	cp -fpr $(HOME)/backfire/package/kaodv/src/* $(HOME)/backfire/build_dir/linux-ar71xx/kaodv/
endef

define Build/Compile
	$(MAKE) -C $(HOME)/backfire/build_dir/linux-ar71xx/linux-2.6.32.27 \
	CC="$(TARGET_CC)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	ARCH="$(ARCH)" \
	SUBDIRS="$(HOME)/backfire/build_dir/linux-ar71xx/kaodv/" \
	EXTRA_CFLAGS="$(TARGET_CFLAGS)" \
	modules
endef

$(eval $(call KernelPackage,kaodv))






