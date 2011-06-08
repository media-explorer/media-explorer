#!/bin/bash
# clutter-keysyms.h to js

file=`basename "$1"`

if [ "$file" != "clutter-keysyms.h" ]; then
echo "Argument 1 should be the path to clutter-keysyms.h"
exit
fi

echo "/*! Clutter
 *
 * Copyright (C) 2006, 2007, 2008  OpenedHand Ltd
 * Copyright (C) 2009, 2010  Intel Corp
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses>.
 */" >> clutter-keysyms.js

while read line; do

iskeydefine=`echo $line | cut -f 1 -d '_'`
if [ "$iskeydefine" == "#define CLUTTER" ]; then
name=`echo $line | cut -f 2 -d ' '`
value=`echo $line | cut -f 3 -d ' '`

echo -n "var $name=$value;" >> clutter-keysyms.js;
fi

done < $1
