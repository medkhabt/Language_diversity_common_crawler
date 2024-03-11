import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional

from extractions.warc_extraction import WarcExtraction 
from extractions.local_extraction import LocalExtraction 
from extractions.extraction import Extraction 
class ExtractionHandler(AbstractHandler): 
    """ 
    Attributes
    ---------- 
    _extraction: extractions.extraction.Extraction 
		The extraction impl used to get the the content.
    """ 
    _extraction:Extraction      
    def __init__(self ):
        """ Construct the ExtractionHandler by setting up the Extraction implementation"""
        self._extraction = { 'warc':  WarcExtraction(), 'local' : LocalExtraction() }  
    def handle(self, request:Any) -> Optional[Any]: 
        """ Fill the request with the information extraction based on the extraction implementation""" 
        logging.info("handling the extraction phase")
        if (('content' in request) and ('type-content' in request) and ('record' in request)):
            if (request['type-content'] == 'warc'): 
                request['meta'] = self._extraction['warc'].meta_extraction(request['content'])
                request['http_header'] = self._extraction['warc'].http_header_extraction(request['record'])
                request['uri'] = self._extraction['warc'].uri_extraction(request['record'])
                request['id'] = self._extraction['warc'].id_extraction(request['record'])
            elif (request['type-content'] == 'local'): 
                request['meta'] = self._extraction['local'].meta_extraction(request['content'])
                request['http_header'] = self._extraction['local'].http_header_extraction(request['record'])
            else : 
                return 1
            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['content'] in the request dict") 

