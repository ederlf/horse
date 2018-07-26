#!/bin/bash

#install deps if we are in travis
if [ -n "$TRAVIS" ]; then
	sudo apt-get update
	sudo apt-get install python-pip
	sudo pip install cython
	sudo apt-get install autoconf libtool build-essential pkg-config
	sudo apt-get install libevent-dev libpcap-dev
	git clone https://github.com/ederlf/libcfluid_base.git && cd $TRAVIS_BUILD_DIR/libcfluid_base && 
	git checkout multi-client && ./autogen.sh && ./configure && make
	sudo make install
	cd $TRAVIS_BUILD_DIR
    git clone https://github.com/google/cmockery.git && cd $TRAVIS_BUILD_DIR/cmockery && ./configure && make 
    sudo make install
    cd $TRAVIS_BUILD_DIR
	./boot.sh && ./configure && make
	cd $TRAVIS_BUILD_DIR/python && python setup.py build_ext --inplace
else
	./boot.sh && ./configure && make
	cd python && python setup.py build_ext --inplace
fi
