import logging
from typing import Any, Optional
from handlers.handler_basis import AbstractHandler
class StatsHandler(AbstractHandler): 
    _stat_init: bool = True ;   
    def handle(self, request:Any) -> Optional[Any] :
        log.info("handling the stats phase")
        if ('language_models' in request): 
           for lang in requests['language_models']: 
               if (lang not in requests): 
                   raise Exception("Couldn't find the language model {lang} the as a key in the request dict") 
           if self._stat_init_ : 
               self._stat_init = False; 
               request['accuracy'] = {}
               request['unknown'] = {} 
           accuracy_stat(request['accuracy'], request) 
           unknowns_stat(request['unknown'], request)      
           return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['language_models'] in the request dict") 


def accuracy_stat(res):
    if(res['detect_fast']['lang'] == res['langid']['lang'] and res['detect_fast']['lang'] != res['cld2']['lang']):
        dic['cld2']['wrong'] =  dic['cld2']['wrong'] + 1 
    elif(res['detect_fast']['lang'] == res['cld2']['lang'] and res['detect_fast']['lang'] != res['langid']['lang'] ):
        dic['langid']['wrong'] = dic['langid']['wrong'] + 1 
    elif(res['langid']['lang'] == res['cld2']['lang'] and res['detect_fast'] != res['langid']['lang'] ):
        dic['detect_fast']['wrong'] = dic['detect_fast']['wrong'] + 1 

    if(res['detect_fast']['lang'] != 'un' and res['detect_fast']['lang'] != 'en' and res['langid']['lang'] == 'en' and res['cld2']['lang'] == 'un') : 
         dic['detect_fast']['uniq'] = dic['detect_fast']['uniq'] + 1
    elif(res['detect_fast']['lang'] == 'un' and res['langid']['lang'] != 'en' and res['cld2']['lang'] == 'un') : 
         dic['langid']['uniq'] = dic['langid']['uniq'] + 1
    elif(res['detect_fast']['lang'] == 'un' and res['langid']['lang'] == 'en' and res['cld2']['lang'] != 'un' and res['cld2']['lang'] != 'en') : 
         dic['cld2']['uniq'] = dic['cld2']['uniq'] + 1
    if( (res['detect_fast']['lang'] == res['langid']['lang'] and res['langid']['lang'] == res['cld2']['lang']) or ( res['detect_fast']['lang'] == 'un' and res['langid']['lang'] == 'en' and res['cld2']['lang'] == 'un') ):
        dic['match'] = dic['match'] + 1


#
def unknowns_stat(dic, res): 
    for key in res.keys():
        if res[key] is not None and type(res[key]) is not str and 'lang' in res[key].keys():
            if(res[key]['lang'] == 'un'): 
                dic[key] = dic[key] + 1 
# TODO check also when a model gives a language but the other two doesn't.
# I assumed that what's unknown for the langid model is 'en', in case one other model couldn't identify it is high probable that the 'en' means unknown in the langid identification.
# This should be generealized if needed, i only take in consideration that we only have 3 identificaiton language models.
def check_accuracy(dic, res):
    if(res['detect_fast']['lang'] == res['langid']['lang'] and res['detect_fast']['lang'] != res['cld2']['lang']):
        dic['cld2']['wrong'] =  dic['cld2']['wrong'] + 1 
    elif(res['detect_fast']['lang'] == res['cld2']['lang'] and res['detect_fast']['lang'] != res['langid']['lang'] ):
        dic['langid']['wrong'] = dic['langid']['wrong'] + 1 
    elif(res['langid']['lang'] == res['cld2']['lang'] and res['detect_fast'] != res['langid']['lang'] ):
        dic['detect_fast']['wrong'] = dic['detect_fast']['wrong'] + 1 

    if(res['detect_fast']['lang'] != 'un' and res['detect_fast']['lang'] != 'en' and res['langid']['lang'] == 'en' and res['cld2']['lang'] == 'un') : 
         dic['detect_fast']['uniq'] = dic['detect_fast']['uniq'] + 1
    elif(res['detect_fast']['lang'] == 'un' and res['langid']['lang'] != 'en' and res['cld2']['lang'] == 'un') : 
         dic['langid']['uniq'] = dic['langid']['uniq'] + 1
    elif(res['detect_fast']['lang'] == 'un' and res['langid']['lang'] == 'en' and res['cld2']['lang'] != 'un' and res['cld2']['lang'] != 'en') : 
         dic['cld2']['uniq'] = dic['cld2']['uniq'] + 1
    if( (res['detect_fast']['lang'] == res['langid']['lang'] and res['langid']['lang'] == res['cld2']['lang']) or ( res['detect_fast']['lang'] == 'un' and res['langid']['lang'] == 'en' and res['cld2']['lang'] == 'un') ):
        dic['match'] = dic['match'] + 1
#



#    accuracy['size'] = accuracy['size'] + 1
#    dataset.append(res)

