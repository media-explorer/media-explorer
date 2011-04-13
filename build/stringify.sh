#! /bin/sh

bname=`basename "${2%}" .h`;
varname=`echo -n "${bname}" | tr -c a-z _`;

echo "static const char ${varname}[] = " 
sed -n \
    -e 's/"/\\"/g' \
    -e 's/^/\t"/' \
    -e 's/$/"/' \
    -e p \
    < "$1"
echo ";"
