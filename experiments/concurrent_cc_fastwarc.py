from fastwarc.warc import ArchiveIterator 
from fastwarc.stream_io import GZipStream
from resiliparse.extract.html2text import extract_plain_text
from resiliparse.parse.lang import detect_fast as d
import concurrent.futures
import langid
from multiprocessing import Process, Event 
import io
from os import path
import requests
import sys
import json
# not used yet.
import time

def main(): 
    wet_url = 'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/warc/CC-MAIN-20230921073711-20230921103711-00001.warc.gz'
#'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/wet/CC-MAIN-20230921073711-20230921103711-00000.warc.wet.gz'

    bundle = [];
    res = requests.get(wet_url); 
    if res.status_code == 200: 
        print(f'Downloaded: {str(wet_url)}')
        save_cc(res)
    else :
        print("Failed")
def fill_dataset(inputs):
    language = language_identification(boilerplate_removal(inputs['content']), 'langid');
    return {
	'uri' : inputs['uri'], 
	'id' : inputs['id'], 
#		'lang' : record.headers.get('WARC-Identified-Content-Language'),
	'lang' : language,
	'len' : inputs['len'] 
#	    'content' : record.reader.read().decode('utf-8')
#	    'content' : record_content 
    }
def boilerplate_removal(content): 
    return extract_plain_text(content, main_content=True); 

def language_identification(content, language_model):
    if language_model == 'detect_fast': 
        return d(content) 
    elif language_model == 'langid' : 
        return langid.classify(content) 
    else:
        return 1;
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
	
# TODO: check for the http_content_type incase the charset doesn't exist. For now i don't find any problems using just the charset. But I don't konw if the results are actually correct. 
def save_cc(res, offset=0, size=10000):
    dataset = [] 
    counter = 0;
    mp_counter = 0;
    batch_counter = 0;
    enc_pr_ctr = 0;
    # Rename the append function of the dataset.
    dataset_append = dataset.append 
    res_bytesio = io.BytesIO(res.content)
    res_stream = GZipStream(res_bytesio)
    mps = []    
    inputs_args = [] 
    these_futures = [] 
    for record in ArchiveIterator(res_bytesio):
        if counter < offset : 
            continue; 
        if counter >= size :
            break;
        if record.http_charset == None or  record.http_charset == 'utf-7': 
            charset = 'utf-8'
        else:
            charset = record.http_charset
        res = decode(record, charset)
        if res == 1 : 
# TODO Probably add a log to the urls that couldn't get decoded.
            enc_pr_ctr = enc_pr_ctr + 1;
            continue;
# TODO don't really need the else
        else: 
#            record_content =  record.reader.read().decode(charset)
            inputs_args.append(
	{
	'uri' : record.headers.get('WARC-Target-URI'),
	'id' : record.headers.get('WARC-Record-ID'),
#		'lang' : record.headers.get('WARC-Identified-Content-Language'),
	'len' : record.headers.get('Content-Length'),
#	    'content' : record.reader.read().decode('utf-8')
	'content' : res.replace("'", "`")
         })
            mp_counter = mp_counter + 1;
#            fill_dataset(record, res); 
                        #    print(f'*********\n header items are : {record.http_headers.items()}')
        if mp_counter >= 64 or counter == (size-1) : 
            print(f"*****************   >> {batch_counter} / {size/64}")
            batch_counter = batch_counter + 1
            with concurrent.futures.ProcessPoolExecutor(max_workers=None) as executor:   
                for input_arg in inputs_args:
                    these_futures.append(executor.submit(fill_dataset, input_arg)) 
                concurrent.futures.wait(these_futures)
            mp_counter = 0;
        counter = counter + 1
    dataset = [future.result() for future in these_futures]
# For the second try we just break and ignore that link
    res_bytesio.close()
    print(f'the processors size is {len(mps)}')
    print(f' We had {enc_pr_ctr} enconding instance problem out of {counter}') 

    with open(f'data/cc/out/test_concurrency', 'w', encoding='utf-8') as f: 
# Found a problem with the max caraters allowed in a single line, the process get killed.
        json.dump(dataset, f, ensure_ascii = False, indent=2)
    print (f'Done: with this shit')

if __name__ == '__main__':
   main()
