import logging
from typing import Any, Optional
from handlers.handler_basis import AbstractHandler
from stats.accuracy import Accuracy 
from stats.unknown import Unknown 
from stats.performance import Performance 

class StatsHandler(AbstractHandler): 
    """ 
    Attributes
    ---------- 
    _stat_init: bool 
	is it the first time that the stathandler is traiting a request
     _stats: dict 
	contains all the stats that we are including in the pipeline and wit their implementations.
    """ 
    _stat_init: bool = True ;   
    _stats: dict
    def __init__(self): 
        """ Initiate the _stats with the implementations"""
        self._stats = {
	    'accuracy' : {'counter' :  {'size':0, 'match':0} , 'instance' : Accuracy()},  
            'unknown' : {'counter' : {},  'instance' :  Unknown()} ,
	    'performance' : {'counter' :{}, 'instance': Performance()}
    } 
    def handle(self, request:Any) -> Optional[Any] :
        """Handle the stats"""
        logging.info("handling the stats phase")
        request['stats'] = {}
        if ('language_models' in request): 
           for lang in request['language_models']: 
               if (lang not in request): 
                   raise Exception("Couldn't find the language model {lang} the as a key in the request dict") 
           if self._stat_init : 
               self.refresh_stats(request);
           for key in self._stats:
               self._stats[key]['instance'].generate(request, self._stats[key]['counter'])
               request['stats'][key] = self._stats[key]
           return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['language_models'] in the request dict") 

    def refresh_stats(self, request): 
        """Initiate the stat holders properly"""
        self._stat_init = False; 
        for lang in request['language_models']:
            for key in self._stats: 
                self._stats[key]['counter'][lang] =  self._stats[key]['instance'].counter_reset();

