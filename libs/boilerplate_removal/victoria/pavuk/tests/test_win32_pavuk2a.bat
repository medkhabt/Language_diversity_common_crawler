rem single thread 'mirror' mode web grab.

rem accept these Special:xxxxxx pages !only! :
rem
rem http://cat.wiki.avid.com/index.php/Special:Lonelypages
rem http://cat.wiki.avid.com/index.php/Special:Unusedimages
rem http://cat.wiki.avid.com/index.php/Special:Allpages
rem http://cat.wiki.avid.com/index.php?title=Special:Recentchanges&hideminor=0&hideliu=0&hidebots=0&hidepatrolled=0&limit=500&days=30&limit=500
rem

rem
rem -fnrules:
rem
rem -fnrules F "*/index.php/*:*" "%h:%r/%d/%n%s.%X"     -- convert 'index.php/Images:xyz.png' HTML pages to 'index.php/Images:xyz.png.html'
rem -fnrules F "*/index.php[/?]*" "%h:%r/%d/%b%s.%X"    -- convert 'index.php/YadaYada' HTML pages to 'index.php/YadaYada.html' 
rem -fnrules F "*" "%h:%r/%d/%b%s.%Y"                   -- keep extensions on any other URL: favorite.ico, style.css, et al
rem

pavuk -verbose -dumpdir pavuk_data/ -noRobots -cdir pavuk_cache/ -cookie_send -cookie_recv -cookie_check -cookie_update -cookie_file pavuk_data/chunky-cookies3.txt -read_css -auto_referer -enable_js -info_dir pavuk_info/ -mode mirror -index_name chunky-index.html -request "URL:http://cat.wiki.avid.com/index.php?title=Special:Recentchanges&hideminor=0&hideliu=0&hidebots=0&hidepatrolled=0&limit=500&days=30&limit=500 METHOD:GET" -request "URL:http://cat.wiki.avid.com/index.php/Special:Lonelypages METHOD:GET" -request "URL:http://cat.wiki.avid.com/index.php/Special:Unusedimages METHOD:GET" -request "URL:http://cat.wiki.avid.com/index.php/Special:Allpages METHOD:GET" -request "URL:http://cat.wiki.avid.com/ METHOD:GET" -scndir pavuk_scenarios/ -dumpscn TestScenario.txt -nthreads 1 -progress_mode 6 -referer -nodump_after -rtimeout 10000 -wtimeout 10000 -timeout 60000 -dumpcmd test_cmd_dumped.txt -debug -debug_level "all,!locks,!mtlock,!cookie,!trace,!dev,!net,!html,!htmlform,!procs,!mtthr,!user,!limits,!hammer,!protos,!protoc,!protod,!bufio,!rules" -store_info -report_url_on_err -tlogfile pavuk_log_timing.txt -dump_urlfd @pavuk_urlfd_dump.txt -dumpfd @pavuk_fd_dump.txt -dump_request -dump_response -logfile pavuk_log_all.txt -slogfile pavuk_log_short.txt -test_id T002 -adomain cat.wiki.avid.com -use_http11 -skip_url_pattern "*oldid=*,*action=edit*,*action=history*,*diff=*,*limit=*,*[/=]User:*,*[/=]User_talk:*,*[^p]/Special:*,*=Special:[^R]*,*.php/Special:[^LUA][^onl][^nul]*,*MediaWiki:*,*Search:*,*Help:*" -tr_str_str "Image:" "" -tr_chr_chr ":\\!&=?" "_" -mime_types_file ../../../mime.types -fnrules F "*/index.php/*:*" "%h:%r/%d/%n%s.%X" -fnrules F "*/index.php[/?]*" "%h:%r/%d/%b%s.%X" -fnrules F "*" "%h:%r/%d/%b%s.%Y" 
