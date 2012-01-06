#!/bin/sh

# public headers should always #include <mex/mex-foo.h> and not
# #include "mex-foo.h"

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
stat=0

test "x$HEADERS" = x && HEADERS=`find $srcdir -maxdepth 1 -name '*.h'`

for x in $HEADERS; do
  #echo "file: $x"
  if grep -q -e 'include.*"' $x; then
    echo "Ooops $x should not use \"\" includes"
    stat=1
  fi
done

exit $stat
