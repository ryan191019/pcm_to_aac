#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dvoom
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/dvoom
	SECTION:=utils
	CATEGORY:=Utilities
  TITLE:=dvoom
  DEPENDS:= +libpthread +libfdk-aac +alsa-lib
endef

define Package/uartd/description
  uart daemon
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./files/montage/* $(STAGING_DIR)/usr/lib
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/arch/mips/include \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/char

define Package/dvoom/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dvoom $(1)/usr/bin/
endef

$(eval $(call BuildPackage,dvoom))
