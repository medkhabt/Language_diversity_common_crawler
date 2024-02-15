from strategies.language_identification.base import Base
from resiliparse.parse.lang import detect_fast as d
class DetectFast(Base): 
    def identify(self, content:str, perf_dic:dict): 
        if perf_dic['perf'] == 1: 
            start = time.process_time()
            for i in range(100):
                d(content)[0]
            duration = time.process_time() - start 
            perf_dic['detect_fast'] = duration  + 1
#            print(f"cpu time for detect_fast : {duration}")
        return {'lang': d(content)[0], 'precision': d(content)[1]}
