#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

PLATFORM_NO_GMAC := 1
PLATFORM_NO_DDR := 1
PLATFORM_NO_I2S := 1
PLATFORM_NO_PWM := 1
PLATFORM_NO_USB_HOST := 1

ifneq ($(wildcard WICED/platform/MCU/BCM4390x/$(HOST_MCU_VARIANT)/$(APPS_CHIP_REVISION)/$(HOST_MCU_VARIANT)$(APPS_CHIP_REVISION).mk),)
include WICED/platform/MCU/BCM4390x/$(HOST_MCU_VARIANT)/$(APPS_CHIP_REVISION)/$(HOST_MCU_VARIANT)$(APPS_CHIP_REVISION).mk
endif # wildcard $(WICED ...)