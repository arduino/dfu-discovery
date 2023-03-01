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
