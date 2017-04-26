#!/bin/sh 
unset f1
unset d1

if [ "$1" != "-f" ] ; then
    echo "use  -f  switch this time\nnext time use svn_sync\nit is safe and does most work for you"
    exit 1
fi

svn --version | grep -E "version (1.7|1.8)"
newSvn="$?"
if [ "$newSvn" = "0" ]; then
    for f in `git status --porcelain| grep '^??' | cut -b 4- `; do
        echo -n .
        svn info $f >/dev/null 2>&1 && git add $f
    done
    echo 
    git status --porcelain | grep '^ D'| cut -b 4- | xargs --no-run-if-empty git rm
else
    git status | sed -n '/# Untracked/,$p'|sed -n '/#^*$/,$p'|awk '$1 == "#" && NR!=1 {print $2}' | 
    xargs -i sh -c "[ -d {}/.svn/ ] && echo {} '--123--456--' || echo {} '--321--654--'" |
    sed 's#^\./#---888---#;s#^#./&#;s#\./---888---#./#;s#^#./&#;s#\(.*\)/\([^ ]\+\) \(--321--654--\)$#\1/.svn/text-base/\2.svn-base \3#' |sed 's# #\ #g' | 
    xargs -i sh -c 'f1=$(expr "{}" : "\(.*\) --321--654--"); d1=$(expr "{}" : "\(.*\) --123--456--"); [ -f "$f1" -o -d "$d1" ] && echo {} ' | sed 's#.svn/text-base/##;s#.svn-base --321--654--##;s# --123--456--##' |
    xargs --no-run-if-empty git add

    git status | awk '$2 == "deleted:" { print $3}'| xargs --no-run-if-empty git rm
    for f in `find . -empty -name text-base | grep '\.svn\/text-base'`; do
        ff=`echo "$f" | sed 's/.svn\/text-base//'`
        [ -z "`ls $ff`" ] && touch $ff/.gitignore
    done
fi

#find .  -empty -name text-base|grep '\.svn\/text-base'| sed 's/.svn\/text-base/.gitignore/' | xargs --no-run-if-empty touch 

find . -name .gitignore | xargs --no-run-if-empty git add

