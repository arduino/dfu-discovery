/*
 * This file is part of dfu-discovery.
 *
 * Copyright 2023 ARDUINO SA (http://www.arduino.cc/)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <dfu.h>
#include <dfu_util.h>

// Those are cmd-line arguments for dfu-util, we provide
// the default value for them
int match_devnum = -1;
int verbose = 0;
char *match_path = NULL;
int match_vendor = -1;
int match_product = -1;
int match_vendor_dfu = -1;
int match_product_dfu = -1;
int match_config_index = -1;
int match_iface_index = -1;
int match_iface_alt_index = -1;
const char *match_iface_alt_name = NULL;
const char *match_serial = NULL;
const char *match_serial_dfu = NULL;

// This provides dfu_malloc without the need to compile
// dfu_file.c and its dependencies.
void *dfu_malloc(size_t size) { return malloc(size); }

// This is the global DFU tree instance used in dfu_util.
struct dfu_if *dfu_root = NULL;

libusb_context *ctx;

const char *libusbOpen() {
  if (ctx != NULL) return NULL;
  int err = libusb_init(&ctx);
  if (err != 0) {
    return libusb_strerror(err);
  }
  return NULL;
}

void libusbClose() {
  //libusb_exit(ctx);
  //ctx = NULL;
}

void freeDfuIf(struct dfu_if *pdfu) {
    libusb_unref_device(pdfu->dev);
    free(pdfu->alt_name);
    free(pdfu->serial_name);
    free(pdfu);
}

void clearDfuRoot() {
  while (dfu_root) {
    struct dfu_if *pdfu = dfu_root;
    dfu_root = dfu_root->next;
    freeDfuIf(pdfu);
  }
}

void dfuProbeDevices() {
  clearDfuRoot();
  probe_devices(ctx);
}

volatile int has_event = 0;

int libusbHandleEvents() {
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  int res = has_event;
  has_event = 0;
  return res;
}

int callback(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data) {
  has_event = 1;
  return 0;
}

libusb_hotplug_callback_handle callbackHandle;

const char *libusbHotplugRegisterCallback() {
  int err = libusb_hotplug_register_callback(
    ctx,
    LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
    LIBUSB_HOTPLUG_NO_FLAGS,
    LIBUSB_HOTPLUG_MATCH_ANY, // Vendor ID
    LIBUSB_HOTPLUG_MATCH_ANY, // Product ID
    LIBUSB_HOTPLUG_MATCH_ANY, // Device Class
    callback,                 // Actual callback function
    NULL,                     // User data
    &callbackHandle
  );
  if (err != 0) {
    return libusb_strerror(err);
  }
  return NULL;
}

void libusbHotplugDeregisterCallback() {
  libusb_hotplug_deregister_callback(ctx, callbackHandle);
}
