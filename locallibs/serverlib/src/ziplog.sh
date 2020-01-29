#!/bin/bash -x

LOGFILEBASENAMES="sirena system daemon logairimp internet jxt"
LASTLOG=3
ARCHDIR=$HOME/archive/oldlog/inv

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

export PATH=$PATH:./

if [ "$1" != "" ] ; then
   cd $1
fi

for iLOG in $LOGFILEBASENAMES ; do
     nLOGS=`ls -l ${iLOG}.??????-??????.log | wc -l`;
     if [ $nLOGS -gt $LASTLOG ] ; then
        tHEAD=`expr ${nLOGS} - ${LASTLOG}`;
        LOG2GZIP=`ls -1 ${iLOG}.??????-??????.log|sort|head -${tHEAD}`;
        for LOG2gz in $LOG2GZIP ; do
           $GZIP -c $LOG2gz > $ARCHDIR/${LOG2gz}.gz;
           rm $LOG2gz;
        done
     fi
done

