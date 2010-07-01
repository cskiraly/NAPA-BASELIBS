#!/bin/sh
mkdir -p m4 config
autoreconf --force -I config -I m4 --install

