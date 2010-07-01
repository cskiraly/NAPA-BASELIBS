#!/bin/bash

version=`cat version.c|cut -d'"' -f2`

tar czf peer-${version}.tgz peer NAPA1peer.cfg *cfg.template README.txt
