#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := WWD_NoOS_Interface

GLOBAL_INCLUDES := .
$(NAME)_SOURCES  := wwd_rtos.c

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

ifneq ($(filter $(HOST_ARCH), ARM_Cortex_M3 ARM_Cortex_M4),)
NOOS_ARCH:=Cortex_M3_M4
endif

$(NAME)_SOURCES  += $(NOOS_ARCH)/noos.c