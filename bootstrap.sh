#!/bin/sh
echo "Bootstrapping autotools ..."
set -v
aclocal
autoconf
automake --add-missing --foreign
