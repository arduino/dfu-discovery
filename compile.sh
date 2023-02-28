DFU_UTIL=dfu-util-0.11

if [ -d "$DFU_UTIL" ];
then
	echo "$DFU_UTIL already downloaded (directory exists)."
else
	echo "Downloading $DFU_UTIL..."
	curl https://dfu-util.sourceforge.net/releases/$DFU_UTIL.tar.gz -s | tar xz

	cp dfu-util-0.11/src/dfuse_mem.c dfu-util_dfuse_mem.c
	cp dfu-util-0.11/src/dfu_util.c dfu-util_dfu_util.c
	cp dfu-util-0.11/src/quirks.c dfu-util_quirks.c
	# for some reason quirks.c has the exec flag set
	chmod -x dfu-util_quirks.c
fi
