#Language identification
from resiliparse.parse.lang import detect_fast as d
import langid
import pycld2 as cld2  
#Boilerplate removal
from resiliparse.extract.html2text import extract_plain_text
from bs4 import BeautifulSoup
import re
import regex
import meta_language
import os
import subprocess

def get_meta_language_2(index, content):
# TODO change the index name, annoying to remove.
    with open(f".{index}.txt", "w") as text_file:
        text_file.write(content)
    result = subprocess.run(["python3", "meta_language.py", "-f", ".{index}.txt"], capture_output=True)
    os.remove(f".{index}.txt")
    return result.stdout.decode("utf-8").strip()
def get_meta_language(content):
    try: 
        soup = BeautifulSoup(content, 'html.parser')
        meta_language = None
        for meta in soup.find_all('meta') : 
            if meta.get('name') == 'language': 
                return meta.get('content')
        if meta_language is None: 
            html_tag = soup.find('html')
            if html_tag is not None: 	 
                meta_language = html_tag.get('lang') if html_tag.get('lang') is not None else None 
        return meta_language 
    except AssertionError as e: 
        print(e) 
        return None
def get_http_language_header_warc(record): 
    http_language_header = record.http_headers.get('Accept-Language') if record.http_headers is not None else None
    http_language_header = http_language_header.split(",")[0] if http_language_header is not None else None
    return http_language_header 

def get_http_language_header_curl(response):
    try:
        #print(str(response.headers))
        #pattern = r"'Content-Language': '([^']+)'"
        #match = re.search(pattern, str(response.headers))
        #print(f"the match is {match}")
        return response.headers['Content-Language'] 
    except KeyError as e :
        #print(f"get_http_language_header_curl error: {e} ")
        return None 
def get_http_vary_header_curl(response): 
    try: 
        return response.headers['Vary']
    except KeyError as e: 
        return None

def get_http_headers_curl(response):
    return { 
       "language": get_http_language_header_curl(response), 
       "vary" :  get_http_vary_header_curl(response)
    }
def boilerplate_removal(content): 
    return extract_plain_text(content, main_content=True); 
def language_identification(content, language_model, perf_dic={'perf':0}):
    if language_model == 'detect_fast': 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                d(content)[0]
            duration = time.process_time() - start 
            perf_dic['detect_fast'] = duration  + 1
#            print(f"cpu time for detect_fast : {duration}")
        return {'lang': d(content)[0], 'precision': d(content)[1]}
    elif language_model == 'langid' : 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                langid.classify(content)[0] 
            duration = time.process_time() - start 
            perf_dic['langid'] = duration + 1
#            print(f"cpu time for langid: {duration}")
        return {'lang': langid.classify(content)[0], 'precision' :  langid.classify(content)[1]}
    elif language_model == 'cld3' : 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                cld3.get_language(content) 
            duration = time.process_time() - start 
            perf_dic['cld3'] = duration + 1
#            print(f"cpu time for cld3: {duration}")
# Language prediction works with normalized probability,
# there is also a is_reliable field we can use it as a threshhold ? if it's not reliable we just return unknown
# The resut is a tuple of (isReliable, textBytesFOUnd, details) 
# the details object is a tuple of up to three languages each one of them in a form of a tuple with 
# details:  Tuple of up to three detected languages, where each is              
#            tuple is (languageName, languageCode, percent, score).  percent is  
#            what percentage of the original text was detected as this language  
#            and score is the confidence score for that language. 

        return {'lang': cld3.get_language(content).language, 'precision' : cld3.get_language(content).probability} 
# We also have a isrelaible field here that we can use a threashold ? 
# TODO Need to check if the first detected languae is the most present in the content.
    elif language_model == 'cld2' : 
        try: 
# source [https://github.com/aboSamoor/polyglot/issues/71#issuecomment-707997790]
            RE_BAD_CHARS = regex.compile(r"[\p{Cc}\p{Cs}]+")
            if perf_dic['perf'] == 1: 
                start = time.process_time()
                for i in range(100):
                    cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][1] 
                duration = time.process_time() - start 
#                print(f"cpu time for cld2: {duration}")
                perf_dic['cld2'] = duration + 1
            return {'lang': cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][1], 'precision': cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][3]} 
        except Exception as e : 
            print(e)
            return 1
    else:
        return 1;
