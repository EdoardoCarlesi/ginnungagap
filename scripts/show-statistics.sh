#!/bin/bash

# Copyright (C) 2010, Steffen Knollmann
# Released under the terms of the GNU General Public License version 3.
# This file is part of `ginnungagap'.

function showForDir ()
{
	find $1 -name \*.[ch] -exec printf "%s\0" {} \; \
		| wc --files0-from=- > $$.1.dat

	find $1 -name \*tests.[ch] -exec printf "%s\0" {} \; \
		| wc --files0-from=- > $$.2.dat

	if [[ x$1 == x. ]]
	then
		echo "Total:"
	else
		echo "$1:"
	fi
	numFilesTotal=$(grep -v total $$.1.dat | wc -l)
	numFilesTest=$(grep -v total $$.2.dat | wc -l)
	numFilesProduction=$(($numFilesTotal - $numFilesTest))
	echo "  Files: $numFilesProduction $numFilesTest $numFilesTotal"
	numLinesTotal=$(grep total $$.1.dat | awk '{print $1}')
	numLinesTest=$(grep total $$.2.dat | awk '{print $1}')
	if [[ ! $numLinesTest ]]
	then
		numLinesTest=0
	fi
	numLinesProduction=$(($numLinesTotal - $numLinesTest))
	echo "  Lines: $numLinesProduction $numLinesTest $numLinesTotal"
	numWordsTotal=$(grep total $$.1.dat | awk '{print $2}')
	numWordsTest=$(grep total $$.2.dat | awk '{print $2}')
	numLinesTest=$(grep total $$.2.dat | awk '{print $1}')
	if [[ ! $numWordsTest ]]
	then
		numWordsTest=0
	fi
	numWordsProduction=$(($numWordsTotal - $numWordsTest))
	echo "  Words: $numWordsProduction $numWordsTest $numWordsTotal"
	numCharsTotal=$(grep total $$.1.dat | awk '{print $3}')
	numCharsTest=$(grep total $$.2.dat | awk '{print $3}')
	numLinesTest=$(grep total $$.2.dat | awk '{print $1}')
	if [[ ! $numCharsTest ]]
	then
		numCharsTest=0
	fi
	numCharsProduction=$(($numCharsTotal - $numCharsTest))
	echo "  Chars: $numCharsProduction $numCharsTest $numCharsTotal"
	echo ""

	rm -rf $$.1.dat $$.2.dat
}

echo ""
echo "This looks (recursively) into the directories and parses all"
echo "files ending on .h and .c to derive some statistical information"
echo "on the size of this project."
echo ""
showForDir src/ginnungagap
showForDir src/libg9p
showForDir src/liblare
showForDir src/libgrid
showForDir src/libdata
showForDir src/libcosmo
showForDir src/libutil
showForDir src
showForDir tools
echo ""
echo "Numbers are: [production code] [test code] [total]"
echo ""
