import logging
from typing import Any, Optional
from handlers.handler_basis import AbstractHandler
class BoilerPlateHandler(AbstractHandler): 
    def handle(self, request:Any) -> Optional[Any] :
        logging.info("handling the boilerplate removal task")
        if 'content' in request: 
            request['content'] = boilerplate_removal(request['content'])
            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['content'] in the request dict") 
def boilerplate_removal(content): 
    return extract_plain_text(content, main_content=True); 
