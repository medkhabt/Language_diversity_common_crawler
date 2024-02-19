from strategies.language_identification.base import Base
import langid
import time
class LangId(Base): 
    def identify(self, content:str, perf_dic:dict): 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                langid.classify(content)[0] 
            duration = time.process_time() - start 
            perf_dic['langid'] = duration 
#            print(f"cpu time for langid: {duration}")
        return {'lang': langid.classify(content)[0], 'precision' :  langid.classify(content)[1]}
