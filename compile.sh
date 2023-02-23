DFU_UTIL=dfu-util-0.11

if [ -d "$DFU_UTIL" ];
then
	echo "$DFU_UTIL already downloaded (directory exists)."
else
	echo "Downloading $DFU_UTIL..."
	curl https://dfu-util.sourceforge.net/releases/$DFU_UTIL.tar.gz -s | tar xz
fi

DEFINES="-DHAVE_UNISTD_H -DHAVE_NANOSLEEP"
INCLUDES="-I. -Idfu-util-0.11/src -I/usr/include/libusb-1.0"
gcc -c dfu-util-0.11/src/dfuse_mem.c $INCLUDES $DEFINES
gcc -c dfu-util-0.11/src/dfu_util.c  $INCLUDES $DEFINES
gcc -c dfu-util-0.11/src/quirks.c    $INCLUDES $DEFINES
g++ -c main.cpp                      $INCLUDES $DEFINES
g++ dfu_util.o quirks.o dfuse_mem.o main.o -lusb-1.0 -o dfu-discovery
