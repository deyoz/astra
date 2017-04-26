#!/bin/sh



if [ "$1" != "-f" ] ; then
    gitst=`git  status -uno --porcelain`

    if [ "$gitst" != "" ] ; then
    echo "$gitst : git not clean"
    exit 1
    fi
else
    shift
fi

if [ "$1" != "" ] ;then
rev="-r $1"
fi
git branch | grep -w master | grep -q '\*' ||  { echo "not on master" ; exit 1 ;}
svn info | tee -a /tmp/svn_sync$$.log
svn up $rev 
./bin/svn_added.sh -f  
rev=`LANG=C svn info .| grep -w Revision:`
echo $rev
git commit -am "svn: $rev"
