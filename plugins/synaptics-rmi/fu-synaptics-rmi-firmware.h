/*
 * Copyright (C) 2019 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include "fu-firmware.h"

#define FU_TYPE_SYNAPTICS_RMI_FIRMWARE (fu_synaptics_rmi_firmware_get_type ())
G_DECLARE_FINAL_TYPE (FuSynapticsRmiFirmware, fu_synaptics_rmi_firmware, FU, SYNAPTICS_RMI_FIRMWARE, FuFirmware)

struct _FuSynapticsRmiFirmware {
	FuFirmware		 parent_instance;
	guint32			 checksum;
	guint8			 io;
	guint8			 bootloader_version;
	guint32			 build_id;
	guint16			 package_id;
	guint16			 product_info;
	gchar			*product_id;
	guint8			 hasBlv5Signature;
	guint32			 blv5SignatureSize;
};

FuFirmware	*fu_synaptics_rmi_firmware_new			(void);
GBytes		*fu_synaptics_rmi_firmware_generate_v0x		(void);
GBytes		*fu_synaptics_rmi_firmware_generate_v10		(void);
