package main

/*
#cgo CPPFLAGS: -DHAVE_UNISTD_H -DHAVE_NANOSLEEP -I. -Idfu-util-0.11/src -I/usr/include/libusb-1.0
#cgo CFLAGS: -DHAVE_UNISTD_H -DHAVE_NANOSLEEP -I. -Idfu-util-0.11/src -I/usr/include/libusb-1.0
#cgo LDFLAGS: -lusb-1.0

#include <dfu.h>
#include <dfu_util.h>
*/
import "C"

func main() {
	C.probe_devices(nil)
}
