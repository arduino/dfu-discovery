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
void dfuProbeDevices();
*/
import "C"

import (
	"fmt"
	"os"
	"time"

	"github.com/arduino/go-properties-orderedmap"
	discovery "github.com/arduino/pluggable-discovery-protocol-handler/v2"
)

func main() {
	dfuDisc := &DFUDiscovery{
		portsCache: map[string]*discovery.Port{},
	}
	disc := discovery.NewServer(dfuDisc)
	if err := disc.Run(os.Stdin, os.Stdout); err != nil {
		fmt.Fprintf(os.Stderr, "Error: %s\n", err.Error())
		os.Exit(1)
	}
}

// DFUDiscovery is the implementation of the DFU pluggable-discovery
type DFUDiscovery struct {
	closeChan  chan<- struct{}
	portsCache map[string]*discovery.Port
}

// Hello is the handler for the pluggable-discovery HELLO command
func (d *DFUDiscovery) Hello(userAgent string, protocolVersion int) error {
	return nil
}

// Quit is the handler for the pluggable-discovery QUIT command
func (d *DFUDiscovery) Quit() {
	d.Stop()
}

// Stop is the handler for the pluggable-discovery STOP command
func (d *DFUDiscovery) Stop() error {
	if d.closeChan != nil {
		close(d.closeChan)
		d.closeChan = nil
	}
	C.libusbClose()
	return nil
}

// StartSync is the handler for the pluggable-discovery START_SYNC command
func (d *DFUDiscovery) StartSync(eventCB discovery.EventCallback, errorCB discovery.ErrorCallback) error {
	if cErr := C.libusbOpen(); cErr != nil {
		return fmt.Errorf("can't open libusb: %s", C.GoString(cErr))
	}
	closeChan := make(chan struct{})
	go func() {
		for {
			d.sendUpdates(eventCB, errorCB)
			select {
			case <-time.After(5 * time.Second):
			case <-closeChan:
				return
			}
		}
	}()
	d.closeChan = closeChan
	return nil
}

func (d *DFUDiscovery) sendUpdates(eventCB discovery.EventCallback, errorCB discovery.ErrorCallback) {
	C.dfuProbeDevices()

	newCache := map[string]*discovery.Port{}
	for _, dfuIf := range d.getDFUInterfaces() {
		newPort := dfuIf.AsDiscoveryPort()
		if _, exist := d.portsCache[newPort.Address]; !exist {
			eventCB("add", newPort)
			newCache[newPort.Address] = newPort
		}
	}

	for _, oldPort := range d.portsCache {
		if _, exist := newCache[oldPort.Address]; !exist {
			eventCB("remove", oldPort)
		}
	}

	d.portsCache = newCache
}

func getPath(dev *C.struct_libusb_device) string {
	return C.GoString(C.get_path(dev))
}

// DFUInterface represents a DFU Interface
type DFUInterface struct {
	Path         string
	AltName      string
	VID, PID     uint16
	SerialNumber string
	Flags        uint32
}

// AsDiscoveryPort converts this DFUInterface into a discovery.Port
func (i *DFUInterface) AsDiscoveryPort() *discovery.Port {
	props := properties.NewMap()
	props.Set("vid", fmt.Sprintf("%04X", i.VID))
	props.Set("pid", fmt.Sprintf("%04X", i.PID))
	if i.SerialNumber != "" {
		props.Set("serial", i.SerialNumber)
	}
	// ProtocolLabel: pdfu.flags & DFU_IFF_DFU ? "DFU" : "Runtime"
	return &discovery.Port{
		Address:       i.Path,
		AddressLabel:  i.Path,
		Protocol:      "dfu",
		ProtocolLabel: "USB DFU",
		Properties:    props,
		HardwareID:    props.Get("serial"),
	}
}

func (d *DFUDiscovery) getDFUInterfaces() []*DFUInterface {
	res := []*DFUInterface{}
	for pdfu := C.dfu_root; pdfu != nil; pdfu = pdfu.next {
		fmt.Println(pdfu)
		if (pdfu.flags & C.DFU_IFF_DFU) == 0 {
			// decide what to do with DFU runtime objects
			// for the time being, ignore them
			continue
		}
		ifPath := getPath(pdfu.dev)
		// for i := 0; i < index; i++ {
		// 	// allow only one object
		// 	if j["ports"][i]["address"] == ifPath {
		// 		index = i
		// 		break
		// 	}
		// }

		dfuIf := &DFUInterface{
			Path:         ifPath,
			AltName:      C.GoString(pdfu.alt_name),
			VID:          uint16(pdfu.vendor),
			PID:          uint16(pdfu.product),
			SerialNumber: C.GoString(pdfu.serial_name),
		}
		res = append(res, dfuIf)
	}
	return res
}
