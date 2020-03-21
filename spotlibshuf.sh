#!/bin/bash


if [ "$1" = "1" ]; then
	LIBFILE=~/.spotlibdump/spotlib_saved_albums.txt
elif [ "$1" = "2" ]; then
	LIBFILE=~/.spotlibdump/spotlib_liked_track_albums.txt
else
	echo "No valid argument specified."
	exit -1
fi



if [ ! -f "$LIBFILE" ]; then
	echo "Library file not found."
	exit -1
fi

echo
echo "-- Recent --"
cat "$LIBFILE" | head -n 20 | sort -R | head -n 5
echo
echo "--- All ----"
cat "$LIBFILE" | sort -R | head -n 5
echo

