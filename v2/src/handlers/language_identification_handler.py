import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
class LanguageIdentificationHandler(AbstractHandler): 
    def handle(self, request:Any) -> Optional[Any]: 
        log.info("handling the language identification phase")
        if ('language_models' in request) and ('content' in request) : 
            for lang in request['language_models'] : 
                lg_id = language_identification(
                   request['content'], 
                   lang, 
                   1 if ('perf_dic' in request) else 0  
                )
            request[lang] = {'lang':'un', 'precision' : lg_id['precision']} if lg_id == 1 or lg_id['lang'] == 'unknown' or len(lg_id) > 2 else  lg_id
            return super().handle(request);
        else: 
            # TODO Refactor the handle of missing args for the handlers.
            raise Exception("Couldn't find one/some/all of the following keys ['language_modles' , 'content'] in the request dict") 



def language_identification(content, language_model, perf_dic):
    if language_model == 'detect_fast': 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                d(content)[0]
            duration = time.process_time() - start 
            perf_dic['detect_fast'] = duration  + 1
#            print(f"cpu time for detect_fast : {duration}")
        return {'lang': d(content)[0], 'precision': d(content)[1]}
    elif language_model == 'langid' : 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                langid.classify(content)[0] 
            duration = time.process_time() - start 
            perf_dic['langid'] = duration + 1
#            print(f"cpu time for langid: {duration}")
        return {'lang': langid.classify(content)[0], 'precision' :  langid.classify(content)[1]}
    elif language_model == 'cld3' : 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                cld3.get_language(content) 
            duration = time.process_time() - start 
            perf_dic['cld3'] = duration + 1
#            print(f"cpu time for cld3: {duration}")
# Language prediction works with normalized probability,
# there is also a is_reliable field we can use it as a threshhold ? if it's not reliable we just return unknown
# The resut is a tuple of (isReliable, textBytesFOUnd, details) 
# the details object is a tuple of up to three languages each one of them in a form of a tuple with 
# details:  Tuple of up to three detected languages, where each is              
#            tuple is (languageName, languageCode, percent, score).  percent is  
#            what percentage of the original text was detected as this language  
#            and score is the confidence score for that language. 

        return {'lang': cld3.get_language(content).language, 'precision' : cld3.get_language(content).probability} 
# We also have a isrelaible field here that we can use a threashold ? 
# TODO Need to check if the first detected languae is the most present in the content.
    elif language_model == 'cld2' : 
        try: 
# source [https://github.com/aboSamoor/polyglot/issues/71#issuecomment-707997790]
            RE_BAD_CHARS = regex.compile(r"[\p{Cc}\p{Cs}]+")
            if perf_dic['perf'] == 1: 
                start = time.process_time()
                for i in range(100):
                    cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][1] 
                duration = time.process_time() - start 
#                print(f"cpu time for cld2: {duration}")
                perf_dic['cld2'] = duration + 1
            return {'lang': cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][1], 'precision': cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][3]} 
        except Exception as e : 
            print(e)
            return 1
    else:
        return 1;

