/*
 * Copyright (C) 2019 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <glib-object.h>

#include "fu-synaptics-rmi-device.h"

typedef enum {
	RMI_V7_PARTITION_ID_NONE			= 0x00,
	RMI_V7_PARTITION_ID_BOOTLOADER			= 0x01,
	RMI_V7_PARTITION_ID_DEVICE_CONFIG,
	RMI_V7_PARTITION_ID_FLASH_CONFIG,
	RMI_V7_PARTITION_ID_MANUFACTURING_BLOCK,
	RMI_V7_PARTITION_ID_GUEST_SERIALIZATION,
	RMI_V7_PARTITION_ID_GLOBAL_PARAMETERS,
	RMI_V7_PARTITION_ID_CORE_CODE,
	RMI_V7_PARTITION_ID_CORE_CONFIG,
	RMI_V7_PARTITION_ID_GUEST_CODE,
	RMI_V7_PARTITION_ID_DISPLAY_CONFIG,
	RMI_V7_PARTITION_ID_EXTERNAL_TOUCH_AFE_CONFIG,
	RMI_V7_PARTITION_ID_UTILITY_PARAMETER,
} RmiV7PartitionId;

typedef enum {
	RMI_V7_FLASH_CMD_IDLE				= 0x00,
	RMI_V7_FLASH_CMD_ENTER_BL,
	RMI_V7_FLASH_CMD_READ,
	RMI_V7_FLASH_CMD_WRITE,
	RMI_V7_FLASH_CMD_ERASE,
	RMI_V7_FLASH_CMD_ERASE_AP,
	RMI_V7_FLASH_CMD_SENSOR_ID,
} RmiV7FlashCommand;

gboolean	 fu_synaptics_rmi_v7_device_write_firmware	(FuDevice	*device,
								 FuFirmware	*firmware,
								 FwupdInstallFlags flags,
								 GError		**error);
gboolean	 fu_synaptics_rmi_v7_device_setup		(FuSynapticsRmiDevice	*self,
								 GError			**error);
gboolean	 fu_synaptics_rmi_v7_device_query_status	(FuSynapticsRmiDevice	*self,
								 GError			**error);
