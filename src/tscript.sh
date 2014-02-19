#!/bin/sh
rm -f *.log
if [ ! -d ./Cores ]; then 
	mkdir ./Cores; 
fi;
rm -f sirena_run_time.txt;
rm -rf Cores/*;

./run -nosir -tscript $@
