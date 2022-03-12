#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Extract header files to a given TARGET_PATH"
	echo "$0 {TARGET_PATH}"
	exit 0
fi

if [ ! -d $1 ]; then
	echo "Target path does not exist, creating a directory $1"
	mkdir -p $1
fi

tar zchf tensorflow-lite.tgz `find tensorflow/lite -name '*.h' -print | grep -v "lite/tools" | grep -v "lite/toco"`
tar zxvf tensorflow-lite.tgz -C $1
