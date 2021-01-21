#!/bin/sh

AWK1='OFS="|" {print '
AWK3=' } '
AWKS="${AWK1}${2}${AWK3}"

sed '1,/BEGINDATA/d' $1 |awk -F'|' "$AWKS"  | tr -d "\"" | iconv -f cp866 -t utf8
