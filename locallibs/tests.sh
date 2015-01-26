#!/bin/sh
summary=$PWD/summary.log
rm -f $summary
touch $summary
for i in $@; do
    tempFile=$PWD/$i.output
    cd $i
    make tests > $tempFile 2>&1
    [ -s tclmon.log ] && cat ./tclmon.log > $PWD/$i.log
    grep "[0-9]\+%: Checks: [0-9]\+" $tempFile | sed -s "s/\(.*\)/$i\t\1/g" >> $summary
    cd -
done
cat $summary
grep -v "100%: Checks:" $summary > /dev/null && exit 1
exit 0
