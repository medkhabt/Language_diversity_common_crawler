import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional

from extractions.extraction import Extraction 
class ContentLengthHandler(AbstractHandler): 
    """ 
    Attributes
    ---------- 
    _extraction: extractions.extraction.Extraction 
		The extraction impl used to get the the content.
    """ 
    def __init__(self ):
        """ Construct the ExtractionHandler by setting up the Extraction implementation"""
        self._extraction = Extraction() 
    def handle(self, request:Any) -> Optional[Any]: 
        """ Fill the request with the information extraction based on the extraction implementation""" 
        logging.info("handling the content length retrieval phase")
        if (('content' in request) and ('type-content' in request)):
            request['length'] = self._extraction.get_content_length(request['content'])
            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['content'] in the request dict") 

