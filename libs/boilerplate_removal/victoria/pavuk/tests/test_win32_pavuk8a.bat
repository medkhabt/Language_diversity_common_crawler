rem test targeted help for invalid command line args

rem 'devel' is an invalid flag identifier : expected targeted help for '-debug_level'

pavuk http://sapos-ntrip.de/ -debug -verbose -debug_level all,!mtlock,!locks,!devel
