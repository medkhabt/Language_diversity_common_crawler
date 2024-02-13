import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
class DecodingHandler(AbstractHandler): 
    _err_counter : int = 0 
    def handle(self, request:Any) -> Any : 
        logging.info("Handling the decoding of the request")
        if 'record' in request: 
# TODO just to test, the func shouldn't be here.
            request['content'] = decode(request['record'], request['charset'])
            # we skip the pipeline for instances that can't be decoded.
            if(request['content'] != 1):
                return super().handle(request) 	
            logging.warning("Decoding Handler::handle >>> couldn't decode the content of the request.")
            self._err_counter = self._err_counter + 1
            return 1; 
        else: 
            raise Exception("Couldn't find one/some/all of the following keys ['record', 'charsert'] in the request dict") 
# TODO remove this from here and put it in a strategy file.
def decode(record): 

    default_encoding = 'iso-8859-1';
    try:
        if record.http_charset == None or  record.http_charset == 'utf-7': 
            charset = 'utf-8'
        else:
            charset = record.http_charset

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
	
