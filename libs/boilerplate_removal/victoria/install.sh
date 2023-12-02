#!/bin/bash
dir=`dirname $0`
cd "$dir"

# config
full=`pwd`
sed -e "s|%prefix%|$full|" <config.dist|sed -e "s|%help%|html.help|" >config

# pavuk
( cd pavuk; ./configure --disable-ssl --prefix="$full"/pavuk-bin; make; make install )

# permissions
chmod a+rwx data demo-data
