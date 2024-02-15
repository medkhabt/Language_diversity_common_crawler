from stats.base  import Base
class Accuracy(Base) : 
# TODO check also when a model gives a language but the other two doesn't.
# I assumed that what's unknown for the langid model is 'en', in case one other model couldn't identify it is high probable that the 'en' means unknown in the langid identification.
# This should be generealized if needed, i only take in consideration that we only have 3 identificaiton language models.
    def generate(self, input_dict: dict, output_dict:dict ) -> None: 
        if(input_dict['detect_fast']['lang'] == input_dict['langid']['lang'] and input_dict['detect_fast']['lang'] != input_dict['cld2']['lang']):
            output_dict['cld2']['wrong'] =  output_dict['cld2']['wrong'] + 1 
        elif(input_dict['detect_fast']['lang'] == input_dict['cld2']['lang'] and input_dict['detect_fast']['lang'] != input_dict['langid']['lang'] ):
            output_dict['langid']['wrong'] = output_dict['langid']['wrong'] + 1 
        elif(input_dict['langid']['lang'] == input_dict['cld2']['lang'] and input_dict['detect_fast'] != input_dict['langid']['lang'] ):
            output_dict['detect_fast']['wrong'] = output_dict['detect_fast']['wrong'] + 1 
        if(input_dict['detect_fast']['lang'] != 'un' and input_dict['detect_fast']['lang'] != 'en' and input_dict['langid']['lang'] == 'en' and input_dict['cld2']['lang'] == 'un') : 
            output_dict['detect_fast']['uniq'] = output_dict['detect_fast']['uniq'] + 1
        elif(input_dict['detect_fast']['lang'] == 'un' and input_dict['langid']['lang'] != 'en' and input_dict['cld2']['lang'] == 'un') : 
           output_dict['langid']['uniq'] = output_dict['langid']['uniq'] + 1
        elif(input_dict['detect_fast']['lang'] == 'un' and input_dict['langid']['lang'] == 'en' and input_dict['cld2']['lang'] != 'un' and input_dict['cld2']['lang'] != 'en') : 
            output_dict['cld2']['uniq'] = output_dict['cld2']['uniq'] + 1
        if( (input_dict['detect_fast']['lang'] == input_dict['langid']['lang'] and input_dict['langid']['lang'] == input_dict['cld2']['lang']) or ( input_dict['detect_fast']['lang'] == 'un' and input_dict['langid']['lang'] == 'en' and input_dict['cld2']['lang'] == 'un') ):
            output_dict['match'] = output_dict['match'] + 1

    def counter_reset(self) -> None: 
        return {'uniq' : 0, 'wrong' : 0}
