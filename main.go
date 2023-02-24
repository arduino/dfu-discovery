package main

/*
#cgo CPPFLAGS: -DHAVE_UNISTD_H -DHAVE_NANOSLEEP -I. -Idfu-util-0.11/src -I/usr/include/libusb-1.0
#cgo CFLAGS: -DHAVE_UNISTD_H -DHAVE_NANOSLEEP -I. -Idfu-util-0.11/src -I/usr/include/libusb-1.0
#cgo LDFLAGS: -lusb-1.0

#include <dfu.h>
#include <dfu_util.h>
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
	return nil
}

// StartSync is the handler for the pluggable-discovery START_SYNC command
func (d *DFUDiscovery) StartSync(eventCB discovery.EventCallback, errorCB discovery.ErrorCallback) error {
	return nil
}
