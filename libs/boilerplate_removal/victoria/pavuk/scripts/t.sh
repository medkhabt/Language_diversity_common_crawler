#!/bin/sh
#    -nouse_http11 \
#    -transparent_proxy p2-sjc-eqx.tnetli.net:9115 \
#    -noverify \
#    -quiet \
 
rm -rf http https

./pavuk \
    -identity "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.0.3705; .NET CLR 1.1.4322)" \
    -noencode \
    -nostore_index \
    -nostore_info \
    -url_strategy pre \
    -cdir /tmp \
    -nocache \
    -mode dontstore \
    -singlepage \
    -noRobots \
    -cookie_recv \
    -cookie_send \
    -noRelocate \
    -nthreads 4 \
    -tlogfile - \
    -unique_sslid \
    -enable_js \
    $*

EXIT_CODE=$?

exit $EXIT_CODE
