rem 0.9.36 FTP error flow / report test

rem
rem Test pavuk [error] flow in case DNS doesn't resolve for FTP site
rem
rem Originally, tl_select() would barf with an error on a sock = -1, which
rem is 'good' (if reports an error), but the code flow is questionable as the
rem DNS resolve should already have caused an activity abort anyway.
rem

pavuk ftp://ftp.cs.titech.ac.jp
rem      -debug -verbose -debug_level all,!mtlock,!locks,!dev,!trace



rem
rem another error test case for the DNS
rem

pavuk ftp://utsun.s.u-tokyo.ac.jp/ftpsync/prep -ftp_passive



rem
rem an ACTIVE FTP test
rem

pavuk ftp://ftp.informatik.rwth-aachen.de/pub/gnu/ -singlepage



rem
rem multiple sites; first does not connect (no DNS error, the FTP server
rem apparently has been removed), the second has some issues with 
rem LIST vs. NLST support for directory listings.
rem
rem NOTE: the second site is a bit 'funny' in that *some* directories
rem       seem to react well to the NLST (ls) command, while others
rem       report emptiness unless you LIST (dir).
rem

pavuk ftp://viz.tamu.edu ftp://svr-ftp.eng.cam.ac.uk -FTPdir -ftp_passive
rem       -debug -verbose -debug_level all,!mtlock,!locks,!dev,!trace
