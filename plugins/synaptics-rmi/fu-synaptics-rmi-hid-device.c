/*
 * Copyright (C) 2020 Richard Hughes <richard@hughsie.com>
 * Copyright (c) 2020 Synaptics Incorporated.
 * Copyright (C) 2012-2014 Andrew Duggan
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "fu-synaptics-rmi-hid-device.h"

struct _FuSynapticsRmiHidDevice {
	FuSynapticsRmiDevice	 parent_instance;
};

G_DEFINE_TYPE (FuSynapticsRmiHidDevice, fu_synaptics_rmi_hid_device, FU_TYPE_SYNAPTICS_RMI_DEVICE)

static void
fu_synaptics_rmi_hid_device_init (FuSynapticsRmiHidDevice *self)
{
}

static void
fu_synaptics_rmi_hid_device_class_init (FuSynapticsRmiHidDeviceClass *klass)
{
}
