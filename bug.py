import json 
import requests 
import os
import time 
from warcio import ArchiveIterator

print("start test")
# the dynamic part.
wet_url = 'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/wet/CC-MAIN-20230921073711-20230921103711-00000.warc.wet.gz' 

r = requests.get(wet_url, stream=True)
records = ArchiveIterator(r.raw) 
i = 0 ; 
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
            content = a.decode('utf-8').replace("'", "`")
            os.system("echo '{}'  > tmp.log | sh commands/lang_id.sh tmp.log ".format(content)); 
            i = i + 1;
    except StopIteration as e: 
        print("end of the file") 
        break

    # STATS ? 


    #os.system("rm tmp.log")
r.close();


