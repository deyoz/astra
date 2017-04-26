#!/bin/bash
for i in $@; do
    (cd $i && make clean)  || exit 1
done

