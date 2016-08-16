# Application module simpleFS

include $(TOPDIR)/rules.mk

PKG_NAME:=simpleFS
PKG_RELEASE:=1.0
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/simpleFS
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=For SimpleFS Applicaion.
endef

define Package/simpleFS/description
	This is a simple file system.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/simpleFS/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/simpleFS $(1)/usr/bin/
endef

$(eval $(call BuildPackage,simpleFS))








