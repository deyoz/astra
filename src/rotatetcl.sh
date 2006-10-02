#!/bin/ksh -x

LOGFILEBASENAMES="astra  daemon  logairimp  monitor  system  tclmon "
LASTLOG=2
ARCHDIR=$HOME/archive/oldlog
ARCHLOGTTL=3 

for i in /usr/local/bin/gzip /bin/gzip /usr/bin/gzip ; do
        if [ -f $i ] ; then
                GZIP=$i
                break
    fi
done
if [ "" == "$GZIP" ] ; then
    echo no GZIP
    exit 1
fi

if [  "$1" != "" ] ; then 
cd $1
fi
export PATH=$PATH:./

dat=`date +\%y\%m\%d-\%H\%M\%S`

./logkilltcl

for iLOG in $LOGFILEBASENAMES ; do
 mv ${iLOG}.moved ${iLOG}.${dat}.log
 nLOGS=`ls -l ${iLOG}.??????-??????.log | wc -l`;
 if [ $nLOGS -ge $LASTLOG ] ; then
    tHEAD=`expr ${nLOGS} - ${LASTLOG}`;
    LOG2GZIP=`ls ${iLOG}.??????-??????.log|head -${tHEAD}`;
    for LOG2gz in $LOG2GZIP ; do
       gzip -c $LOG2gz > $ARCHDIR/${LOG2gz}.gz;
       rm $LOG2gz;
    done
  fi
done
find $ARCHDIR -type f -name "*.??????-??????.log.gz" -follow -mtime +${ARCHLOGTTL} -exec rm {} \;

#hour=`date +\%H`

#core eraser
#$HOME/bin/chkcore.sh $1 > $1/chkore.log 2>&1



