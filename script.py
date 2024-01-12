import json 
import requests 
import os
import time 
import sys
from warcio import ArchiveIterator

print("start test")
os.system("rm commands/result_cc.log")
output_name="{}_{}_{}".format(sys.argv[2],sys.argv[1],sys.argv[3])
# the dynamic part.
wet_url = 'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/wet/CC-MAIN-20230921073711-20230921103711-{}.warc.wet.gz'.format(sys.argv[3]) 

bundle = [];
r = requests.get(wet_url, stream=True)
records = ArchiveIterator(r.raw) 
i = 0;
# TODO I need to map the language identification of the tool with the header that presents the lang.
while (True):
    try:
        record = next(records)
        if record is None: 
            print("break")
            break;	 
        if i == 1000 : 
            print("1000 mark")
        if record.rec_type == 'conversion' : 
            a = record.content_stream().read()
            lang = record.rec_headers.get_header('warc-identified-content-language'); 
            content = "{}::{}::{}\n {}".format(
		record.rec_headers.get_header('WARC-Target-URI'),
		"" if ( lang == None) else lang.split(',', 1)[0], 
		record.rec_headers.get_header('Content-Length'),
		a.decode('utf-8').replace("'", "`")
            )	
# How to concat the results of the resulted log file with the bundle.
# "python cc_boilerplate_removal.py '{}' >
# 
#            os.system("python cc_boilerplate_removal.py '{}' > tmp.log | bash commands/lang_id_{}.sh tmp.log {}".format(content, sys.argv[1], output_name)); 
            os.system("echo '{}' > tmp.log | bash commands/lang_id_{}.sh tmp.log {}".format(content, sys.argv[1], output_name)); 
            i = i + 1;
    except StopIteration as e: 
        print("end of the file") 
        break

os.system("rm *.tmp")
os.system("rm tmp.log")
#WARC-Target-URI
#WARC-Identified-Content-Language
#Content-Length

r.close();
