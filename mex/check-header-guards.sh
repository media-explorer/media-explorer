#!/bin/sh

# Checks if a header is protected by #ifdef/#define/#endif guards that respect
# the __HEADER_H__ convention

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
stat=0

test "x$HEADERS" = x && HEADERS=`find $srcdir -maxdepth 1 -name '*.h'`
test "x$SOURCES" = x && SOURCES=`find $srcdir -maxdepth 1 -name '*.c'`

SKIP="mex-marshal.h"

for x in $HEADERS; do
  xx=`echo "$x" | sed 's@.*/@@'`
  echo $SKIP | grep $xx 1>/dev/null 2>/dev/null && continue
  tag=`echo "$xx" | tr 'a-z.-' 'A-Z_'`
  #echo "file: $x, tag: $tag"
  lines=`grep "\<__${tag}__\>" "$x" | wc -l | sed 's/ 	//g'`
  if test "x$lines" != x3; then
    echo "Ooops, header file $x does not have correct preprocessor guards"
    stat=1
  fi
done

exit $stat
