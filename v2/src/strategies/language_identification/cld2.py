from strategies.language_identification.base import Base
import pycld2 as cld2  
import regex
import time
class CLD2(Base): 
    def identify(self, content:str, perf_dic:dict): 
        try: 
# source [https://github.com/aboSamoor/polyglot/issues/71#issuecomment-707997790]
            RE_BAD_CHARS = regex.compile(r"[\p{Cc}\p{Cs}]+")
            if perf_dic['perf'] == 1: 
                start = time.process_time()
                for i in range(100):
                    cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][1] 
                duration = time.process_time() - start 
                perf_dic['cld2'] = duration
#                print(f"cpu time for cld2: {duration}")
            return {'lang': cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][1], 'precision': cld2.detect(RE_BAD_CHARS.sub("", content))[2][0][3]} 
        except Exception as e : 
            print(e)
            return 1
