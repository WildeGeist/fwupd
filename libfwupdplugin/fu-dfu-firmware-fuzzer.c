/*
 * Copyright (C) 2020 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include "fu-fuzzer.h"
#include "fu-dfu-firmware.h"

int
LLVMFuzzerTestOneInput (const guint8 *data, gsize size)
{
	g_autoptr(FuFirmware) firmware = fu_dfu_firmware_new ();
	g_autoptr(GBytes) fw = g_bytes_new (data, size);
	fu_firmware_parse (firmware, fw,
			   FWUPD_INSTALL_FLAG_NO_SEARCH |
			   FWUPD_INSTALL_FLAG_IGNORE_VID_PID |
			   FWUPD_INSTALL_FLAG_IGNORE_CHECKSUM,
			   NULL);
	return 0;
}
