import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional

from extractions.warc_extraction import WarcExtraction 
from extractions.extraction import Extraction 
class ExtractionHandler(AbstractHandler): 
    _extraction:Extraction      
    def __init__(self):
        self._extraction = WarcExtraction()  
    def handle(self, request:Any) -> Optional[Any]: 
        logging.info("handling the extraction phase")
        if (('content' in request) and ('record' in request)): 
            request['meta'] = self._extraction.meta_extraction(request['content'])
            request['http_header'] = self._extraction.http_header_extraction(request['record'])
            request['uri'] = self._extraction.uri_extraction(request['record'])
            request['id'] = self._extraction.id_extraction(request['record'])
            request['len'] = self._extraction.length_content_extraction(request['record'])

            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['content'] in the request dict") 

