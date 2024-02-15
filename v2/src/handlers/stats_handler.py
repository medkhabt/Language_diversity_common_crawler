import logging
from typing import Any, Optional
from handlers.handler_basis import AbstractHandler
from stats.accuracy import Accuracy 
from stats.unknown import Unknown 
class StatsHandler(AbstractHandler): 
    _stat_init: bool = True ;   
    _stats: dict
    def __init__(self): 
        self._stats = {
	    'accuracy' : {'counter' :  {'size':0, 'match':0} , 'instance' : Accuracy()},  
            'unknown' : {'counter' : {},  'instance' :  Unknown()} 
    } 
    def handle(self, request:Any) -> Optional[Any] :
        logging.info("handling the stats phase")
        if ('language_models' in request): 
           for lang in request['language_models']: 
               if (lang not in request): 
                   raise Exception("Couldn't find the language model {lang} the as a key in the request dict") 
           if self._stat_init : 
               self.refresh_stats(request);
           for key in self._stats:
               self._stats[key]['instance'].generate(request, self._stats[key]['counter'])
               request[key] = self._stats[key]['counter']   
           return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['language_models'] in the request dict") 

    def refresh_stats(self, request): 
        self._stat_init = False; 
        for lang in request['language_models']:
            for key in self._stats: 
                self._stats[key]['counter'][lang] =  self._stats[key]['instance'].counter_reset();

