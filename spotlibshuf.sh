#!/bin/bash

ALBFILE=~/.spotlibdump/spotlib_saved_albums.txt

if [ ! -f "$ALBFILE" ]; then
	echo "Album file not found."
	exit -1
fi

echo
echo "-- Recent --"
cat "$ALBFILE" | head -n 20 | sort -R | head -n 5
echo
echo "--- All ----"
cat "$ALBFILE" | sort -R | head -n 5
echo

