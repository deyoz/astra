#!/bin/bash

for dir in $(ls -d */);  do
  if [[ "$dir" = "mespro/" ]] ; then
    continue;
  fi;

  cd "$dir" > /dev/null
  ls | grep -vE "bz2|gz" | xargs rm -rf
  cd - > /dev/null
done
