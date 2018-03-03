#!/bin/bash

#install deps if we are in travis
if [ -n "$TRAVIS" ]; then
	sudo apt-get install python-pip
	sudo pip install cython
	sudo apt-get install autoconf libtool build-essential pkg-config
	sudo apt-get install libevent-dev
	git clone https://github.com/ederlf/libcfluid_base.git && cd $TRAVIS_BUILD_DIR/libcfluid_base
	./autogen && ./configure && make
	sudo make install
	cd $TRAVIS_BUILD_DIR
fi

./boot.sh && ./configure && make
cd $TRAVIS_BUILD_DIR/python && python setup.py build_ext --inplace

