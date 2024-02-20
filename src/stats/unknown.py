from stats.base  import Base
class Unknown(Base) : 
    def generate(self, input_dict: dict, output_dict:dict ) -> None: 
        for lang in input_dict['language_models']:   
            if(input_dict[lang]['lang'] == 'un'): 
                output_dict[lang] = output_dict[lang] + 1 
    def counter_reset(self) -> None: 
        return 0
    def format(self, counter, size) ->str: 
       return f"{counter['detect_fast']* 100 / size}% {counter['langid']* 100 / size}% {counter['cld2']* 100 / size}%\n"
