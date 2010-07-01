#!/bin/bash

wget -O - http://${1:-repository.napa-wine.eu:9832}/Manage?action=clear 
