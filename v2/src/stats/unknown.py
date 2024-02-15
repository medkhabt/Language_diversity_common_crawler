from stats.base  import Base
class Unknown(Base) : 
    def generate(self, input_dict: dict, output_dict:dict ) -> None: 
        for lang in input_dict['language_models']:   
            if(input_dict[lang]['lang'] == 'un'): 
                output_dict[lang] = output_dict[lang] + 1 
    def counter_reset(self) -> None: 
        return 0
