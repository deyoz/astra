#!/bin/bash
set -euo pipefail

COPYTO_EDITYPE_DIR=${1?destination directory as the 1st parameter}
EDITYPE_FILENAME=${2?source .dat file as the 2nd parameter}
TMPDIR=`mktemp -d /tmp/edi_tmp_XXXXXX`

trap "rm -r $TMPDIR" EXIT

OUT_BASE_FILENAME=`basename $EDITYPE_FILENAME .dat`
OUT_FILE1="$OUT_BASE_FILENAME.etp"
OUT_FILE2="edi_$OUT_BASE_FILENAME.h"

`dirname $0`/msgtypes.awk -v BASEDIR=$TMPDIR -v BASEFILENAME=$OUT_BASE_FILENAME $EDITYPE_FILENAME

if( ! diff $TMPDIR/$OUT_FILE1 $COPYTO_EDITYPE_DIR/$OUT_FILE1 > /dev/null ) ; then
    cp $TMPDIR/$OUT_FILE1 $COPYTO_EDITYPE_DIR/$OUT_FILE1;
fi
if( ! diff $TMPDIR/$OUT_FILE2 $COPYTO_EDITYPE_DIR/$OUT_FILE2 > /dev/null ) ; then
    cp $TMPDIR/$OUT_FILE2 $COPYTO_EDITYPE_DIR/$OUT_FILE2;
fi

