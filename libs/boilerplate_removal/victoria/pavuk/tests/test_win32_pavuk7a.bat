rem 0.9.36 crash report test

rem
rem This site produces some VERY non-RFC-compliant HTTP response messages, e.g.
rem   'SOURCETABLE 200 OK'
rem instead of
rem   'HTTP/1.1 200 OK'
rem

pavuk http://sapos-ntrip.de/

rem      -debug -verbose -debug_level all,!mtlock,!locks,!dev,!trace
