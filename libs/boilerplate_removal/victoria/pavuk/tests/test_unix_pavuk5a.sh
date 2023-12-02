#! /bin/sh

# triple thread 'mirror' mode web grab.

../src/pavuk -x -ewait -verbose -dumpdir pavuk_data/ -noRobots -cdir pavuk_cache/ -cookie_send -cookie_recv -cookie_check -cookie_update -cookie_file pavuk_data/chunky-cookies3.txt -read_css -auto_referer -enable_js -info_dir pavuk_info/ -mode mirror -index_name chunky-index.html -request "URL:http://cat.wiki.avid.com/ METHOD:GET" -scndir pavuk_scenarios/ -dumpscn TestScenario.txt -nthreads 1 -progress_mode 6 -referer -nodump_after -rtimeout 10000 -wtimeout 10000 -timeout 60000 -dumpcmd test_cmd_dumped.txt -debug_level "!all,!locks,!mtlock,!cookie,!trace,!dev,!net,!html,!procs,!mtthr,!user,limits,!hammer,!protos,!protoc,!protod,!bufio" -store_info -report_url_on_err -tlogfile pavuk_log_timing.txt -dump_urlfd @pavuk_urlfd_dump.txt -dumpfd @pavuk_fd_dump.txt -dump_request -dump_response -logfile pavuk_log_all.txt -slogfile pavuk_log_short.txt -test_id T002  -adomain cat.wiki.avid.com -use_http11 -skip_url_pattern '*oldid=*,*action=edit*,*action=history*,*diff=*,*limit=*,*/User:*,*/User_talk:*,*Special:*' -tr_str_str 'Image:' '' -tr_chr_chr ':\\!&' '_'



