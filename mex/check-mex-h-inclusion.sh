#!/bin/sh

# mex.h should not be included by the main library to reduce dependencies when
# recompiling. Individual headers should be included instead

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
stat=0

test "x$HEADERS" = x && HEADERS=`find $srcdir -maxdepth 1 -name '*.h'`
test "x$SOURCES" = x && SOURCES=`find $srcdir -maxdepth 1 -name '*.c'`

SKIP="mex-test-internal.c"

for x in $HEADERS $SOURCES; do
  xx=`echo "$x" | sed 's@.*/@@'`
  echo $SKIP | grep -q $xx && continue
  #echo "file: $x"
  if grep -q mex\\.h $x; then
    echo "Ooops $x includes mex.h directly"
    stat=1
  fi
done

exit $stat
