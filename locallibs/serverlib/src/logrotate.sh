#!/bin/bash

filename=${1%.*}
dat=`date +\%y\%m\%d-\%H\%M\%S`

mv -f $1 $filename.$dat.log


