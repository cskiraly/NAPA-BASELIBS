#! /bin/bash
# stop and cleanup some virtual machines of a lab
# if no parameters are given the whole lab stoped
# if VM names are given, only those are stopped
lcrash -q $@ && lclean $@
