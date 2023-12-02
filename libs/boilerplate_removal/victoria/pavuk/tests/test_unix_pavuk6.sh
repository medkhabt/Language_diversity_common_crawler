#! /bin/sh

#
# test if default -dont_leave_site setting (TRUE!) works; 0.6.35 and before had a bug where
# default settings for cfg.conditions.xxx elements were b0rked due to sizeof(long)!=sizeof(int)
# on 64-bit boxes.
#
# This 
#
../src/pavuk https://securepostplaza.tntpost.nl/tracktrace/ -debug -SSL -debug_level !all,html,limits -nohttp -report_url_on_err

