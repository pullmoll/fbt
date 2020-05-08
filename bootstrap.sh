#!/bin/sh
echo "Bootstrapping autotools ..."
set -v
aclocal
autoconf
libtoolize
automake --add-missing --foreign
