#WARC Extraction
from fastwarc.warc import ArchiveIterator 
from fastwarc.stream_io import GZipStream


#Boilerplate removal
from resiliparse.extract.html2text import extract_plain_text

#Language identification
from resiliparse.parse.lang import detect_fast as d
import langid
import pycld2 as cld2  

#Util in Traitement
from bs4 import BeautifulSoup
import regex 
import io
from os import path
import requests
import sys
import json
# not used yet.
import time

import util

def main(): 
    if len(sys.argv) > 1: 
        perf_calc = 1 if sys.argv[1] == '1' else 0;
    else: 
        perf_calc = 0 
    if len(sys.argv) > 2 and len(sys.argv[2]) == 5 : 
        seg_number = sys.argv[2]
    else: 
        seg_number = '00000'
    print(f"args are : [ perf_calc : {perf_calc} , seg_number = {seg_number} ]")
    warc_url = f'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/warc/CC-MAIN-20230921073711-20230921103711-{seg_number}.warc.gz'
    bundle = [];
    res = requests.get(warc_url); 
    if res.status_code == 200: 
        print(f'Downloaded: {str(warc_url)}')
        save_cc(res, perf=perf_calc, seg_number = seg_number)
    else :
        print("Failed")
def decode(record, charset): 
    default_encoding = 'iso-8859-1';
    try:
        record_content = record.reader.read().decode(charset)
        if  record_content == None: 
            return 1; 
        return record_content;
    except UnicodeDecodeError :
        if charset == default_encoding:
#	    we need to skip the url
            return 1;
        elif charset == 'utf-8' or charset == None or  record.http_charset == 'utf-7': 
            return decode(record, default_encoding); 
        elif charset == 'gbk' : 
# source https://stackoverflow.com/questions/3218014/unicodeencodeerror-gbk-codec-cant-encode-character-illegal-multibyte-sequen 
            return decode(record, 'gb18030')
        elif charset == 'shift_jis': 
# source https://stackoverflow.com/questions/6729016/decoding-shift-jis-illegal-multibyte-sequence
            return decode(record, 'shift_jisx0213')
        elif charset == 'euc-jp': 
# source https://stackoverflow.com/questions/73255012/python-fails-to-decode-euc-jp-strings-with-the-character 
            return decode(record, 'euc-jisx0213');
        elif charset == 'windows-1251': 
            return decode(record, 'utf-8')
        else: 
            return 1;
	
def save_cc(res, seg_number='00000', perf=0 ,offset=0, size=1000000):
    dataset = [] 
    counter = 0;
    enc_pr_ctr = 0;
    res_bytesio = io.BytesIO(res.content)
    res_stream = GZipStream(res_bytesio)
    dataset = []
    for record in ArchiveIterator(res_bytesio):
        if counter < offset : 
            continue; 
        if counter >= size :
            break;
        if record.http_charset == None or  record.http_charset == 'utf-7': 
            charset = 'utf-8'
                #print(f'default to utf-8')
        else:
            charset = record.http_charset
                #print(f'we have a charset info which is {charset}')                   
        res = decode(record, charset)
        if res == 1 : 
# TODO Probably add a log to the urls that couldn't get decoded.
            enc_pr_ctr = enc_pr_ctr + 1;
            continue;
# TODO don't really need the else
        else: 
            fill_dataset(dataset, record, res)
                        #    print(f'*********\n header items are : {record.http_headers.items()}')
        counter = counter + 1
# For the second try we just break and ignore that link
    res_bytesio.close()
    print(f' We had {enc_pr_ctr} enconding instance problem out of {counter}') 
#TODO all of this part is hardcoded for now, in case we want to experiment with other language identfication models we need to refactor this for a smoother experience :D 

    with open(f'logs/comp_{seg_number}.log', 'w', encoding='utf-8') as f: 
# Found a problem with the max caraters allowed in a single line, the process get killed.
#        json.dump(dataset, f, ensure_ascii = False, indent=2)
        f.write(f"meta_warc|http_header_warc|lang_warc|meta_curl|http_header_curl|lang_curl\n")
        for dr in dataset  :
            f.write(f"{dr['warc']['meta']}|{dr['warc']['http_header']}|{dr['warc']['lang']['lang']}|{dr['curl']['meta']}|{dr['curl']['http_header']}|{dr['curl']['lang']['lang']}\n")



def fill_dataset(dataset, record, content):
####### WARC PART
    # META LANGUAGE INFO  
    res = {'warc' : {}, 'curl': {}} 
    meta_language = util.get_meta_language(content)
    # HTTP LANGUAGE HEADER 
#TODO change the way to get the langugae header so we can refactor this part when curling .
    http_language_header = util.get_http_language_header_warc(record); 
    language_identification_models = ['detect_fast'] 
    for lang_id_mdl in language_identification_models: 
        lg_id_warc = util.language_identification(util.boilerplate_removal(content), lang_id_mdl) 
    res['warc'] =  {
	    'uri' : record.headers.get('WARC-Target-URI'),
	    'id' : record.headers.get('WARC-Record-ID'),
	    'len' : record.headers.get('Content-Length'),
	    'http_header' : http_language_header if http_language_header is not None else '-',
	    'meta' : meta_language if meta_language is not None else '-',
            'lang' : {'lang':'un', 'precision' : 0} if lg_id_warc == 1 or lg_id_warc['lang'] == 'unknown' or len(lg_id_warc) > 2 else  lg_id_warc

}
######## CURL PART
    ## curl the uri 
    warc_dic = res['warc']
    if warc_dic['uri'] is None:
        return 1
#    print(f"the info of the warc file is {res['warc']}")
    try: 
#        headers = {"Accept-Language": "en-US, en; q=0.5"}
        headers = {"Accept-Language": None}
        r = requests.get(res['warc']['uri'], headers=headers)
    except Exception as e: 
#        print(e)
        return 1; 
    content = r.text
    http_content_header = util.get_http_language_header_curl(r)
    for lang_id_mdl in language_identification_models: 
        lg_id_curl = util.language_identification(util.boilerplate_removal(content), lang_id_mdl) 
    res['curl'] = {
         'http_header': headers["Accept-Language"] if headers["Accept-Language"] is not None else '-',
         'http_content_header' : http_content_header if http_content_header is not None else '-' , 
         'meta':  util.get_meta_language(content), 
         'lang':  {'lang':'un', 'precision' : 0} if lg_id_curl == 1 or lg_id_curl['lang'] == 'unknown' or len(lg_id_curl) > 2 else  lg_id_curl
 
    } 
#    print(f"r['headers'] = {r.headers}")
#    print(f"res['curl'] = {res['curl']}")
## detect_fast uses unknown instead of un.


    dataset.append(res)

main()
