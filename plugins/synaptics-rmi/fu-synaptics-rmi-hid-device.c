/*
 * Copyright (C) 2020 Richard Hughes <richard@hughsie.com>
 * Copyright (c) 2020 Synaptics Incorporated.
 * Copyright (C) 2012-2014 Andrew Duggan
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <sys/ioctl.h>
#include <linux/hidraw.h>

#include "fu-io-channel.h"

#include "fu-synaptics-rmi-hid-device.h"

struct _FuSynapticsRmiHidDevice {
	FuSynapticsRmiDevice	 parent_instance;
	FuIOChannel		*io_channel;
};

G_DEFINE_TYPE (FuSynapticsRmiHidDevice, fu_synaptics_rmi_hid_device, FU_TYPE_SYNAPTICS_RMI_DEVICE)

#define RMI_WRITE_REPORT_ID				0x9	/* output report */
#define RMI_READ_ADDR_REPORT_ID				0xa	/* output report */
#define RMI_READ_DATA_REPORT_ID				0xb	/* input report */
#define RMI_ATTN_REPORT_ID				0xc	/* input report */
#define RMI_SET_RMI_MODE_REPORT_ID			0xf	/* feature report */

#define RMI_DEVICE_DEFAULT_TIMEOUT			2000

#define HID_RMI4_REPORT_ID				0
#define HID_RMI4_READ_INPUT_COUNT			1
#define HID_RMI4_READ_INPUT_DATA			2
#define HID_RMI4_READ_OUTPUT_ADDR			2
#define HID_RMI4_READ_OUTPUT_COUNT			4
#define HID_RMI4_WRITE_OUTPUT_COUNT			1
#define HID_RMI4_WRITE_OUTPUT_ADDR			2
#define HID_RMI4_WRITE_OUTPUT_DATA			4
#define HID_RMI4_FEATURE_MODE				1
#define HID_RMI4_ATTN_INTERUPT_SOURCES			1
#define HID_RMI4_ATTN_DATA				2

static GByteArray *
fu_synaptics_rmi_hid_device_read (FuSynapticsRmiDevice *rmi_device, guint16 addr, gsize req_sz, GError **error)
{
	FuSynapticsRmiHidDevice *self = FU_SYNAPTICS_RMI_HID_DEVICE (rmi_device);
	g_autoptr(GByteArray) buf = g_byte_array_new ();
	g_autoptr(GByteArray) req = g_byte_array_new ();

	/* maximum size */
	if (req_sz > 0xffff) {
		g_set_error_literal (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "data to read was too long");
		return NULL;
	}

	/* report then old 1 byte read count */
	fu_byte_array_append_uint8 (req, RMI_READ_ADDR_REPORT_ID);
	fu_byte_array_append_uint8 (req, 0x0);

	/* address */
	fu_byte_array_append_uint16 (req, addr, G_LITTLE_ENDIAN);

	/* read output count */
	fu_byte_array_append_uint16 (req, req_sz, G_LITTLE_ENDIAN);

	/* request */
	for (guint j = req->len; j < 21; j++)
		fu_byte_array_append_uint8 (req, 0x0);
	if (g_getenv ("FWUPD_SYNAPTICS_RMI_VERBOSE") != NULL) {
		fu_common_dump_full (G_LOG_DOMAIN, "ReportWrite",
				     req->data, req->len,
				     80, FU_DUMP_FLAGS_NONE);
	}
	if (!fu_io_channel_write_byte_array (self->io_channel, req, RMI_DEVICE_DEFAULT_TIMEOUT,
					     FU_IO_CHANNEL_FLAG_SINGLE_SHOT |
					     FU_IO_CHANNEL_FLAG_USE_BLOCKING_IO, error))
		return NULL;

	/* keep reading responses until we get enough data */
	while (buf->len < req_sz) {
		guint8 input_count_sz = 0;
		g_autoptr(GByteArray) res = NULL;
		res = fu_io_channel_read_byte_array (self->io_channel, req_sz,
						     RMI_DEVICE_DEFAULT_TIMEOUT,
						     FU_IO_CHANNEL_FLAG_SINGLE_SHOT,
						     error);
		if (res == NULL)
			return NULL;
		if (res->len == 0) {
			g_set_error_literal (error,
					     FWUPD_ERROR,
					     FWUPD_ERROR_INTERNAL,
					     "response zero sized");
			return NULL;
		}
		if (g_getenv ("FWUPD_SYNAPTICS_RMI_VERBOSE") != NULL) {
			fu_common_dump_full (G_LOG_DOMAIN, "ReportRead",
					     res->data, res->len,
					     80, FU_DUMP_FLAGS_NONE);
		}

		/* ignore non data report events */
		if (res->data[HID_RMI4_REPORT_ID] != RMI_READ_DATA_REPORT_ID) {
			g_debug ("ignoring report with ID 0x%02x",
				 res->data[HID_RMI4_REPORT_ID]);
			continue;
		}
		if (res->len < HID_RMI4_READ_INPUT_DATA) {
			g_set_error (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "response too small: 0x%02x",
				     res->len);
			return NULL;
		}
		input_count_sz = res->data[HID_RMI4_READ_INPUT_COUNT];
		if (input_count_sz == 0) {
			g_set_error_literal (error,
					     FWUPD_ERROR,
					     FWUPD_ERROR_INTERNAL,
					     "input count zero");
			return NULL;
		}
		if (input_count_sz + (guint) HID_RMI4_READ_INPUT_DATA > res->len) {
			g_set_error (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "underflow 0x%02x from expected 0x%02x",
				     res->len, (guint) input_count_sz + HID_RMI4_READ_INPUT_DATA);
			return NULL;
		}
		g_byte_array_append (buf,
				     res->data + HID_RMI4_READ_INPUT_DATA,
				     input_count_sz);

	}
	if (g_getenv ("FWUPD_SYNAPTICS_RMI_VERBOSE") != NULL) {
		fu_common_dump_full (G_LOG_DOMAIN, "DeviceRead", buf->data, buf->len,
				     80, FU_DUMP_FLAGS_NONE);
	}

	return g_steal_pointer (&buf);
}

static gboolean
fu_synaptics_rmi_hid_device_write (FuSynapticsRmiDevice *rmi_device, guint16 addr, GByteArray *req, GError **error)
{
	FuSynapticsRmiHidDevice *self = FU_SYNAPTICS_RMI_HID_DEVICE (rmi_device);
	guint8 len = 0x0;
	g_autoptr(GByteArray) buf = g_byte_array_new ();

	/* check size */
	if (req != NULL) {
		if (req->len > 0xff) {
			g_set_error_literal (error,
					     FWUPD_ERROR,
					     FWUPD_ERROR_INTERNAL,
					     "data to write was too long");
			return FALSE;
		}
		len = req->len;
	}

	/* report */
	fu_byte_array_append_uint8 (buf, RMI_WRITE_REPORT_ID);

	/* length */
	fu_byte_array_append_uint8 (buf, len);

	/* address */
	fu_byte_array_append_uint16 (buf, addr, G_LITTLE_ENDIAN);

	/* optional data */
	if (req != NULL)
		g_byte_array_append (buf, req->data, req->len);

	/* pad out to 21 bytes for some reason */
	for (guint i = buf->len; i < 21; i++)
		fu_byte_array_append_uint8 (buf, 0x0);
	if (g_getenv ("FWUPD_SYNAPTICS_RMI_VERBOSE") != NULL) {
		fu_common_dump_full (G_LOG_DOMAIN, "DeviceWrite", buf->data, buf->len,
				     80, FU_DUMP_FLAGS_NONE);
	}

	return fu_io_channel_write_byte_array (self->io_channel, buf, RMI_DEVICE_DEFAULT_TIMEOUT,
					       FU_IO_CHANNEL_FLAG_SINGLE_SHOT |
					       FU_IO_CHANNEL_FLAG_USE_BLOCKING_IO,
					       error);
}

static gboolean
fu_synaptics_rmi_hid_device_wait_for_attr (FuSynapticsRmiDevice *rmi_device,
				       guint8 source_mask,
				       guint timeout_ms,
				       GError **error)
{
	FuSynapticsRmiHidDevice *self = FU_SYNAPTICS_RMI_HID_DEVICE (rmi_device);
	g_autoptr(GTimer) timer = g_timer_new ();

	/* wait for event from hardware */
	while (g_timer_elapsed (timer, NULL) * 1000.f < timeout_ms) {
		g_autoptr(GByteArray) res = NULL;
		g_autoptr(GError) error_local = NULL;

		/* read from fd */
		res = fu_io_channel_read_byte_array (self->io_channel,
						     HID_RMI4_ATTN_INTERUPT_SOURCES + 1,
						     timeout_ms,
						     FU_IO_CHANNEL_FLAG_NONE,
						     &error_local);
		if (res == NULL) {
			if (g_error_matches (error_local, G_IO_ERROR, G_IO_ERROR_TIMED_OUT))
				break;
			g_propagate_error (error, g_steal_pointer (&error_local));
			return FALSE;
		}
		if (g_getenv ("FWUPD_SYNAPTICS_RMI_VERBOSE") != NULL) {
			fu_common_dump_full (G_LOG_DOMAIN, "ReportRead",
					     res->data, res->len,
					     80, FU_DUMP_FLAGS_NONE);
		}
		if (res->len < HID_RMI4_ATTN_INTERUPT_SOURCES + 1) {
			g_debug ("attr: ignoring small read of %u", res->len);
			continue;
		}
		if (res->data[HID_RMI4_REPORT_ID] != RMI_ATTN_REPORT_ID) {
			g_debug ("attr: ignoring invalid report ID 0x%x",
				 res->data[HID_RMI4_REPORT_ID]);
			continue;
		}

		/* success */
		if (source_mask & res->data[HID_RMI4_ATTN_INTERUPT_SOURCES])
			return TRUE;

		/* wrong mask */
		g_debug ("source mask did not match: 0x%x",
			 res->data[HID_RMI4_ATTN_INTERUPT_SOURCES]);
	}

	/* urgh */
	g_set_error_literal (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_NOT_SUPPORTED,
			     "no attr report, timed out");
	return FALSE;
}

typedef enum {
	HID_RMI4_MODE_MOUSE				= 0,
	HID_RMI4_MODE_ATTN_REPORTS			= 1,
	HID_RMI4_MODE_NO_PACKED_ATTN_REPORTS		= 2,
} FuSynapticsRmiHidMode;

static gboolean //FIXME HID
fu_synaptics_rmi_hid_device_set_mode (FuSynapticsRmiHidDevice *self,
				  FuSynapticsRmiHidMode mode,
				  GError **error)
{
	const guint8 data[] = { 0x0f, mode };
	if (g_getenv ("FWUPD_SYNAPTICS_RMI_VERBOSE") != NULL)
		fu_common_dump_raw (G_LOG_DOMAIN, "SetMode", data, sizeof(data));
	return fu_udev_device_ioctl (FU_UDEV_DEVICE (self),
				     HIDIOCSFEATURE(sizeof(data)), (guint8 *) data,
				     NULL, error);
}

static gboolean
fu_synaptics_rmi_hid_device_open (FuUdevDevice *device, GError **error)
{
	FuSynapticsRmiHidDevice *self = FU_SYNAPTICS_RMI_HID_DEVICE (device);

	/* set up touchpad so we can query it */
	self->io_channel = fu_io_channel_unix_new (fu_udev_device_get_fd (device));
	if (!fu_synaptics_rmi_hid_device_set_mode (self, HID_RMI4_MODE_ATTN_REPORTS, error))
		return FALSE;

	/* success */
	return TRUE;
}

static gboolean
fu_synaptics_rmi_hid_device_close (FuUdevDevice *device, GError **error)
{
	FuSynapticsRmiHidDevice *self = FU_SYNAPTICS_RMI_HID_DEVICE (device);
	g_autoptr(GError) error_local = NULL;

	/* turn it back to mouse mode */
	if (!fu_synaptics_rmi_hid_device_set_mode (self, HID_RMI4_MODE_MOUSE, &error_local)) {
		/* if just detached for replug, swallow error */
		if (!g_error_matches (error_local,
				      FWUPD_ERROR,
				      FWUPD_ERROR_PERMISSION_DENIED)) {
			g_propagate_error (error, g_steal_pointer (&error_local));
			return FALSE;
		}
		g_debug ("ignoring: %s", error_local->message);
	}

	fu_udev_device_set_fd (device, -1);
	g_clear_object (&self->io_channel);
	return TRUE;
}

static gboolean
fu_synaptics_rmi_hid_device_probe (FuUdevDevice *device, GError **error)
{
	return fu_udev_device_set_physical_id (device, "hid", error);
}

static void
fu_synaptics_rmi_hid_device_init (FuSynapticsRmiHidDevice *self)
{
	fu_device_set_name (FU_DEVICE (self), "Touchpad");
	fu_device_set_remove_delay (FU_DEVICE (self), FU_DEVICE_REMOVE_DELAY_RE_ENUMERATE);
	fu_device_set_version_format (FU_DEVICE (self), FWUPD_VERSION_FORMAT_TRIPLET);
}

static void
fu_synaptics_rmi_hid_device_class_init (FuSynapticsRmiHidDeviceClass *klass)
{
	FuSynapticsRmiDeviceClass *klass_rmi = FU_SYNAPTICS_RMI_DEVICE_CLASS (klass);
	FuUdevDeviceClass *klass_udev = FU_UDEV_DEVICE_CLASS (klass);
	klass_udev->probe = fu_synaptics_rmi_hid_device_probe;
	klass_udev->open = fu_synaptics_rmi_hid_device_open;
	klass_udev->close = fu_synaptics_rmi_hid_device_close;
	klass_rmi->write = fu_synaptics_rmi_hid_device_write;
	klass_rmi->read = fu_synaptics_rmi_hid_device_read;
	klass_rmi->wait_for_attr = fu_synaptics_rmi_hid_device_wait_for_attr;
}
