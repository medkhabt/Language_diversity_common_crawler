import logging
from typing import Any, Optional
from handlers.handler_basis import AbstractHandler
from strategies.boilerplate_removal.boilerplate_removal_resiliparse_html2text import ResiliParseHtml2Text
from strategies.boilerplate_removal.boilerplate_removal import BoilerPlateRemoval 

class BoilerPlateHandler(AbstractHandler): 
    """ 
    Attributes
    ---------- 
    _impl: strategies.boilerplate_removal.boilerplate_removal.BoilerPlateRemoval 
    		The implementation of the BoilerplateRemoval technique.	
    """ 
    _impl:BoilerPlateRemoval
    def __init__(self):
       """ Construct the BoilerPlateHandler by setting up the boilerplate removal implementation"""
       self._impl = ResiliParseHtml2Text()
    def handle(self, request:Any) -> Optional[Any] :
        """ Check if the necessary args exists before starting the boilerplate removal process."""
        logging.info("handling the boilerplate removal task")
        if 'content' in request: 
            request['content'] = self._impl.apply(request['content'])
            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['content'] in the request dict") 
