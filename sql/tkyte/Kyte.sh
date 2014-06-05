SCRIPTS=`echo $0|sed -e 's#\(.*\)/.*#\1/#'`

if [ -z "$SCRIPTS" ] ; then
SCRIPTS=.
fi

#check for dirs not exists
if [ ! -d "1Tab" ] ; then
  if [ -z "$2" ] || [ "$2" != "--force" ] ; then
    echo "SQL directories (1Tab,...) is not found. Check pwd"
    echo "  (to create new: `basename $0` ouser/opass --force)"
    exit 1;
  fi
fi

DIRS="0DataTypes 1Tab 1Tind 2Constr 1Tiot 1Tmview 1Tmview_ind \
2Comment 3Seq 5Proc 7Proc-body 7Function \
7Procedure 2View 8Find 8Trig"
for i in $DIRS ; do
    if [ ! -d $i ] ; then mkdir $i ; fi
    for f in  $i/*.sql  ; do echo $f ; done |  xargs rm -f
done

if [ -z "$1" ]; then
echo "Usage: `basename $0` ouser/opass";
exit;
fi

sqlplus $1 @$SCRIPTS/get_lst.sql

cat data_types.lst| awk 'NF==1 {print $1}'| \
while read i; do
    echo @$SCRIPTS/getdatatypes $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat tables.lst|  awk 'NF==1 {print $1}'| \
while read i; do
echo  @$SCRIPTS/gettab $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat indexes.lst|  awk 'NF==2 {print $1, $2}'| \
while read i t; do
echo @$SCRIPTS/getind $i $t 1Tind;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat iots.lst|  awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getiot $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat constr.lst| grep -v "rows selected"| awk 'NF==4 {print $1, $2, $3, $4}'| \
while read c t i; do
    echo @$SCRIPTS/getconstr $c $t $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat procedures.lst|  awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getprocedure $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql < /dev/null

cat functions.lst |  awk 'NF==1 {print $1}'| \
while read i; do
echo  @$SCRIPTS/getfunction $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

grep -v '\$' packages.lst |  awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getpackage $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql < /dev/null

grep -v '\$' packages.lst |  awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getpackagebody $i;
done >$$-work.sql
sqlplus $1 @$$-work.sql < /dev/null

cat triggers.lst|awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getrig.sql $i
done>$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat triggers.lst|awk 'NF==1 {print $1}'| \
while read i; do
ed 8Trig/$i.sql <<EOF
\$
?;
+1,\$d
a
/
.
w
q
EOF
done

cat views.lst |awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getaview.sql $i
done>$$-work.sql
sqlplus $1 @$$-work.sql < /dev/null

cat mviews.lst |awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getmview.sql $i
done>$$-work.sql
sqlplus $1 @$$-work.sql < /dev/null

cat mv_indexes.lst|  awk 'NF==2 {print $1, $2}'| \
while read i t; do
echo @$SCRIPTS/getind $i $t 1Tmview_ind;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat func_indexes.lst|  awk 'NF==2 {print $1, $2}'| \
while read i t; do
echo @$SCRIPTS/getind $i $t 8Find;
done >$$-work.sql
sqlplus $1 @$$-work.sql </dev/null

cat sequences.lst|awk 'NF==1 {print $1}'| \
while read i; do
echo @$SCRIPTS/getseq.sql $i
done>$$-work.sql
sqlplus $1 @$$-work.sql < /dev/null
rm $$-work.sql
