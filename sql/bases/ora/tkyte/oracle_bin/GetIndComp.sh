SCRIPTS=`echo $0|sed -e 's#\(.*\)/.*#\1/#'`

if [ -z "$SCRIPTS" ] ; then
SCRIPTS=.
fi

DIRS="IndComp"

for i in $DIRS ; do
    if [ ! -d $i ] ; then mkdir $i ; fi
    for f in  $i/*.sql  ; do echo $f ; done |  xargs rm -f
done

if [ -z "$1" ]; then 
echo "Usage: `basename $0` ouser/opass";
exit;
fi

sqlplus $1 @$SCRIPTS/get_lst.sql


cat indexes.lst|  awk 'NF==2 {print $1, $2}'| \
while read i t; do
echo @$SCRIPTS/getindforcomp $i $t;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

SHELLS=$$-shell.sh
echo > $SHELLS
for i in IndComp/*sql ; do
awk -v file=$i '/REM \+-\+-\+/ {print "mv",file,"IndComp/" $3 }' $i >>$SHELLS
done
sh $SHELLS

