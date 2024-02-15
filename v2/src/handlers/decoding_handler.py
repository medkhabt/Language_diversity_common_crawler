import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
from strategies.decoding.decoding import Decoding
class DecodingHandler(AbstractHandler): 
    _err_counter : int = 0 
    _decoder:Decoding = None
    def __init__(self): 
        self._decoder = Decoding();
    def handle(self, request:Any) -> Any : 
        logging.info("Handling the decoding of the request")
        if 'record' in request: 
# TODO just to test, the func shouldn't be here.
            request['content'] = self._decoder.decode(request['record'])
            # we skip the pipeline for instances that can't be decoded.
            if(request['content'] != 1):
                return super().handle(request) 	
            logging.warning("Decoding Handler::handle >>> couldn't decode the content of the request.")
            self._err_counter = self._err_counter + 1
            return 1; 
        else: 
            raise Exception("Couldn't find one/some/all of the following keys ['record', 'charsert'] in the request dict") 
