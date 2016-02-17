#!/bin/sh
existinGit=`find . -name .git -type d`
if [ "$existinGit" ] ; then
    echo "found existing git repos:\n$existinGit"
    exit 1
fi

git init
#svn list  . | grep -v '.*/' | xargs  git add  

(echo . ; (find .  -type d | egrep -v '\.(svn|git)'  | \
while read aaa ; do
 if [ -d $aaa/.svn ] ; then
 echo $aaa 
 svn pg svn:externals $aaa | grep '^\^' | awk -v dir=$aaa '{print dir "/" $2 }'
 fi
done  ))  |  sort -u | \

while read  aaa ; do 
( #echo $aaa >>chdirlist2
 cd  $aaa ;
   svn list -R  . | grep -v '.*/$' | xargs --no-run-if-empty git add -f
) 
done 
git commit -am "init"


