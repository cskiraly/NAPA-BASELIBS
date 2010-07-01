#! /bin/bash
# restart some virtual machines of a lab
# if no parameters are given the whole lab is restarted
# if VM names are given, only those are restarted
lcrash -q $@ && lclean $@ && lstart $@ -f
