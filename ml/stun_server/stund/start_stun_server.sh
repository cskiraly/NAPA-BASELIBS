#!/bin/sh

gksudo ifconfig eth0:1 192.168.0.1 netmask 255.255.255.0 up;

./server -v -h 127.0.0.1 -a 192.168.0.1