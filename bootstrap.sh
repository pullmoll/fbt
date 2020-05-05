#!/bin/sh
echo "Bootstrapping autotools ..."
set -v
aclocal
autoconf
autoheader
automake --add-missing --foreign
