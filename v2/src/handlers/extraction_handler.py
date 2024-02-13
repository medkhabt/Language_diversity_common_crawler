import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
class ExtractionHandler(AbstractHandler): 
    def handle(self, request:Any) -> Optional[Any]: 
        log.info("handling the extraction phase")
        if (('content' in request) and ('record' in request)): 
            request['meta'] = meta_extraction(request['content'])
            request['http_header'] = http_header_extraction(request['record'])
            request['uri'] = uri_extraction(request['record'])
            request['id'] = id_extraction(request['record'])
            request['len'] = length_content_extraction(request['record'])

            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['content'] in the request dict") 

def uri_extraction(record): 
    return record.headers.get('WARC-Target-URI')
def id_extraction(record): 
    return record.headers.get('WARC-Record-ID')
def length_content_extraction(record): 
    return record.headers.get('Content-Length')

def meta_extraction(content):
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
            else : 
                meta_language = '-'
        return meta_language
    except AssertionError as e: 
        print(e) 
        return None
 
def http_header_extraction(record) : 
    http_language_header = record.http_headers.get('Accept-Language') if record.http_headers is not None else None
    http_language_header =  http_language_header.split(",")[0] if http_language_header is not None else '-';  
    return http_language_header if http_language_header is not None else '-'; 
def uri_extraction(record): 
    return record.headers.get('WARC-Target-URI')
def id_extraction(record): 
    return record.headers.get('WARC-Record-ID')
def length_content_extraction(record): 
    return record.headers.get('Content-Length')

