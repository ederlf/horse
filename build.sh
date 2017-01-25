#!/bin/bash

make

cd /home/vagrant/Horse/python && : && python setup.py build_ext --inplace

