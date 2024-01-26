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

def main(): 
    warc_url = 'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/warc/CC-MAIN-20230921073711-20230921103711-00001.warc.gz'

    bundle = [];
    res = requests.get(warc_url); 
    if res.status_code == 200: 
        print(f'Downloaded: {str(warc_url)}')
        save_cc(res)
    else :
        print("Failed")
def unknowns(dic, res): 
    for key in res.keys():
        if(res[key] == 'un'): 
            dic[key] = dic[key] + 1 
# TODO check also when a model gives a language but the other two doesn't.
# This should be generealized if needed, i only take in consideration that we only have 3 identificaiton language models.
def check_accuracy(dic, res):
    if(res['detect_fast'] == res['langid'] and res['detect_fast'] != res['cld2']):
        dic['cld2'] = dic['cld2'] + 1 
    elif(res['detect_fast'] == res['cld2'] and res['detect_fast'] != res['langid'] ):
        dic['langid'] = dic['langid'] + 1 
    elif(res['langid'] == res['cld2'] and res['detect_fast'] != res['langid'] ):
        dic['detect_fast'] = dic['detect_fast'] + 1 
# dataset : the dict containing the info needed from warc
# record : the recode object representing one warc file. 
# content: raw html page 
# accuracy : dict containng the accuracy of a language identification model
def fill_dataset(dataset, record, content, accuracy, unknowns_dic):
    # META LANGUAGE INFO  
    try: 
        soup = BeautifulSoup(content, 'html.parser')
        meta_language = None
        for meta in soup.find_all('meta') : 
            if meta.get('name') == 'language': 
                meta_language = meta.get('content')
        if meta_language is None: 
            html_tag = soup.find('html')
            if html_tag is not None: 	 
                meta_language = html_tag.get('lang') if html_tag.get('lang') is not None else None 
    except AssertionError as e: 
        print(e) 
        meta_language = None
        # HTTP LANGUAGE HEADER 
    http_language_header = record.http_headers.get('Accept-Language') if record.http_headers is not None else None
    http_language_header =  http_language_header.split(",")[0] if http_language_header is not None else None
    language_identification_models = ['detect_fast', 'langid', 'cld2'] 
    res = {
	'uri' : record.headers.get('WARC-Target-URI'),
	'id' : record.headers.get('WARC-Record-ID'),
	'len' : record.headers.get('Content-Length'),
        'http_header' : http_language_header if http_language_header is not None else '-',
        'meta' : meta_language if meta_language is not None else '-'
    }
    lang_idents = []
    for lang_id_mdl in language_identification_models: 
        lg_id = language_identification(boilerplate_removal(content), lang_id_mdl) 
## detect_fast uses unknown instead of un.
        res[lang_id_mdl] = 'un' if lg_id == 1 or lg_id == 'unknown' else  lg_id
    check_accuracy(accuracy, res)
    unknowns(unknowns_dic, res)
    dataset.append(res)
def boilerplate_removal(content): 
    return extract_plain_text(content, main_content=True); 

def language_identification(content, language_model):
    if language_model == 'detect_fast': 
        return d(content)[0]
    elif language_model == 'langid' : 
        return langid.classify(content)[0] 
    elif language_model == 'cld3' : 
        return cld3.get_language(content) 
    elif language_model == 'cld2' : 
        try: 
# source [https://github.com/aboSamoor/polyglot/issues/71#issuecomment-707997790]
            RE_BAD_CHARS = regex.compile(r"[\p{Cc}\p{Cs}]+")
            return cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][1] 
        except Exception as e : 
            print(e)
            return 1
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
    enc_pr_ctr = 0;
    # Rename the append function of the dataset.
    dataset_append = dataset.append 
    res_bytesio = io.BytesIO(res.content)
    res_stream = GZipStream(res_bytesio)
    dataset = []
    accuracy = {'detect_fast' : 0, 'langid' : 0, 'cld2' : 0}
    unknowns_dic = {'detect_fast' : 0, 'langid' : 0, 'cld2' : 0}
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
            fill_dataset(dataset, record, res,accuracy,unknowns_dic)
                        #    print(f'*********\n header items are : {record.http_headers.items()}')
        counter = counter + 1
# For the second try we just break and ignore that link
    res_bytesio.close()
    print(f' We had {enc_pr_ctr} enconding instance problem out of {counter}') 

    with open(f'logs/test', 'w', encoding='utf-8') as f: 
# Found a problem with the max caraters allowed in a single line, the process get killed.
#        json.dump(dataset, f, ensure_ascii = False, indent=2)
        for dr in dataset  :
            f.write(f"{dr['meta']} {dr['http_header']} {dr['detect_fast']} {dr['langid']} {dr['cld2']}\n")
    with open(f'logs/test_accuracy', 'w', encoding='utf-8') as f: 
        f.write(f"{accuracy['detect_fast']} {accuracy['langid']} {accuracy['cld2']}")

    with open(f'logs/test_unknowns', 'w', encoding='utf-8') as f: 
        f.write(f"{unknowns_dic['detect_fast']} {unknowns_dic['langid']} {unknowns_dic['cld2']}")

    print (f'Done: ')
main()
