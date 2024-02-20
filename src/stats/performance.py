from stats.base  import Base
import logging
class Performance(Base) : 
    def generate(self, input_dict: dict, output_dict:dict ) -> None: 
        for lang in input_dict['language_models']:   
            output_dict[lang] =  output_dict[lang] + input_dict['perf_dic'][lang]
    def counter_reset(self) -> None: 
        return 0
    def format(self, counter, size) ->str: 
       return f"{counter['detect_fast']/size} {counter['langid']/size} {counter['cld2']/size} \n"
