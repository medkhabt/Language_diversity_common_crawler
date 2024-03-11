import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
from strategies.decoding.decoding import Decoding
class DecodingHandler(AbstractHandler): 
    """ 
    Attributes
    ---------- 
    _err_counter: int 
		The number of request that couldn't be traited due to an encoding problem.
    _decoder : strategies.decoding.decoding.Decoding 
		The decoder impl used to decode the content.
    """ 
    _err_counter : int = 0 
    _decoder:Decoding = None
    def __init__(self): 
        """ Construct the DecoderHandler by setting up the Decoding implementation"""
        self._decoder = Decoding();
    def handle(self, request:Any) -> Any : 
        """ Handle the decoding response from the implementatoin, count the number of instances that had decoding failures  and check the if the necessary args are present.""" 

        logging.info("Handling the decoding of the request")
        if 'record' in request: 
            #request['content'] = self._decoder.decode(request['record'])
            content = self._decoder.decode(request['record'])
            request['content'] = content
            if(request['content'] != 1):
                return super().handle(request) 	
            logging.warning("Decoding Handler::handle >>> couldn't decode the content of the request.")
            self._err_counter = self._err_counter + 1
            return 1; 
        else: 
            raise Exception("Couldn't find one/some/all of the following keys ['record', 'charsert'] in the request dict") 
