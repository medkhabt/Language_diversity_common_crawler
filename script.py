import json 
import requests 
import os
import time 
from warcio import ArchiveIterator

print("start test")
os.system("rm commands/result.log")
# the dynamic part.
wet_url = 'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/wet/CC-MAIN-20230921073711-20230921103711-00000.warc.wet.gz' 

bundle = [];
r = requests.get(wet_url, stream=True)
records = ArchiveIterator(r.raw) 
i = 0;
while (i < 19):
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
            content = "{} :: {} :: {} \n{} ".format(
		record.rec_headers.get_header('WARC-Target-URI'),
		"" if ( lang == None) else lang.split(',', 1)[0], 
		record.rec_headers.get_header('Content-Length'),	
		a.decode('utf-8').replace("'", "`")
            )	
# How to concat the results of the resulted log file with the bundle.
            os.system("echo '{}'  > tmp.log | sh commands/lang_id.sh tmp.log ".format(content)); 
            i = i + 1;
            time.sleep(0.1);
    except StopIteration as e: 
        print("end of the file") 
        break


#WARC-Target-URI
#WARC-Identified-Content-Language
#Content-Length

print(bundle)
r.close();


