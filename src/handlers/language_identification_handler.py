import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
from strategies.language_identification.cld2 import CLD2
from strategies.language_identification.detect_fast import DetectFast 
from strategies.language_identification.langid import LangId 



class LanguageIdentificationHandler(AbstractHandler): 
    """ 
    Attributes
    ---------- 
    _lang_identifier: dict { '{name_lang_id_model}' : {Lang_id_instance}  }
    		The dict of implementations of the language identification models. 
    """ 
    def __init__(self):  
       """ Initiate the instances of the language identification models. """
       self._lang_identifier = {
	    'cld2' : CLD2(), 
	    'langid' : LangId(), 
	    'detect_fast' : DetectFast()
	} 
    def handle(self, request:Any) -> Optional[Any]: 
        """ 
		- Identify the language of the content in the request with all the language identification models that re specified in _lang_identifier. 
		- Run performance tests if specified.
		- Post-trait the result of the langage identification for special cases.
        """
        logging.info("handling the language identification phase")
        if ('language_models' in request) and ('content' in request) : 
            for lang in request['language_models'] : 
                lg_id = self._lang_identifier[lang].identify(
                   request['content'], 
                   request['perf_dic'] 
                )
                request[lang] = {'lang':'un', 'precision' : '0'} if lg_id == 1 or lg_id['lang'] == 'unknown' or len(lg_id) > 2 else  lg_id
            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['language_modles' , 'content'] in the request dict") 


