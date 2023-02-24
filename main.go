package main

/*
#cgo CPPFLAGS: -DHAVE_UNISTD_H -DHAVE_NANOSLEEP -I. -Idfu-util-0.11/src -I/usr/include/libusb-1.0
#cgo CFLAGS: -DHAVE_UNISTD_H -DHAVE_NANOSLEEP -I. -Idfu-util-0.11/src -I/usr/include/libusb-1.0
#cgo LDFLAGS: -lusb-1.0

#include <dfu.h>
#include <dfu_util.h>
// This is missing from dfu_util.h
char *get_path(libusb_device *dev);

// Defined in main.c
const char *libusbOpen();
void libusbClose();
*/
import "C"

import (
	"fmt"
	"os"

	discovery "github.com/arduino/pluggable-discovery-protocol-handler/v2"
)

func main() {
	dfuDisc := &DFUDiscovery{}
	disc := discovery.NewServer(dfuDisc)
	if err := disc.Run(os.Stdin, os.Stdout); err != nil {
		fmt.Fprintf(os.Stderr, "Error: %s\n", err.Error())
		os.Exit(1)
	}
}

// DFUDiscovery is the implementation of the DFU pluggable-discovery
type DFUDiscovery struct {
}

// Hello is the handler for the pluggable-discovery HELLO command
func (d *DFUDiscovery) Hello(userAgent string, protocolVersion int) error {
	return nil
}

// Quit is the handler for the pluggable-discovery QUIT command
func (d *DFUDiscovery) Quit() {
}

// Stop is the handler for the pluggable-discovery STOP command
func (d *DFUDiscovery) Stop() error {
	C.libusbClose()
	return nil
}

// StartSync is the handler for the pluggable-discovery START_SYNC command
func (d *DFUDiscovery) StartSync(eventCB discovery.EventCallback, errorCB discovery.ErrorCallback) error {
	if cErr := C.libusbOpen(); cErr != nil {
		return fmt.Errorf("can't open libusb: %s", C.GoString(cErr))
	}
	return nil
}

func getPath(dev *C.struct_libusb_device) string {
	return C.GoString(C.get_path(dev))
}
