import os 
import logging
class FileRepository: 
    def clean(self, seg_number:str) : 
        os.system(f'rm ./../logs/test_refactor{seg_number}.log');
    def save(self, seg_number, requests, first_save=False):
        logging.warning("inside the save in file function")
	#TODO all of this part is hardcoded for now, in case we want to experiment with other language identfication models we need to refactor this for a smoother experience :D 
        with open(f'../logs/test_refactor{seg_number}.log', 'a', encoding='utf-8') as f: 
	# Found a problem with the max caraters allowed in a single line, the process get killed.
    #        json.dump(dataset, f, ensure_ascii = False, indent=2)
            if(first_save):
                f.write(f"meta|http_header|detectfast|langid|cld2|pre-detectfast|pre-langid|pre-cld2\n")
            for dr in requests:
                f.write(f"{dr['meta']}|{dr['http_header']}|{dr['detect_fast']['lang']}|{dr['langid']['lang']}|{dr['cld2']['lang']}|{dr['detect_fast']['precision']}|{dr['langid']['precision']}|{dr['cld2']['precision']}\n")
# TODO with saving data before traversing ever instance we need to be careful with the stats.
 #       with open(f'../logs/test_refactor{seg_number}_accuracy.log', 'w', encoding='utf-8') as f: 
 #           f.write("amount of different prediction than the other two language models\n")
 #           f.write(f"{accuracy['detect_fast']['wrong'] * 100/accuracy['size']}% {accuracy['langid']['wrong'] * 100 / accuracy['size']}% {accuracy['cld2']['wrong']* 100 / accuracy['size']}%\n")
 #           f.write("amount of prediction with unknown prediction in the other two models.\n")
 #           f.write(f"{accuracy['detect_fast']['uniq']* 100 / accuracy['size']}% {accuracy['langid']['uniq']* 100 / accuracy['size']}% {accuracy['cld2']['uniq']* 100 / accuracy['size']}%\n")
	
#            f.write(f"the number of perfect matches are: {accuracy['match']*100/accuracy['size']}%")

#        with open(f'../logs/test_refactor{seg_number}_unknowns', 'w', encoding='utf-8') as f: 
#            f.write(f"{unknowns_dic['detect_fast']* 100 / accuracy['size']}% {unknowns_dic['langid']* 100 / accuracy['size']}% {unknowns_dic['cld2']* 100 / accuracy['size']}%\n")

#        with open(f'../logs/test_refactor{seg_number}_performance', 'w', encoding='utf-8') as f: 
#            f.write(f"{perf_dict['detect_fast']} {perf_dict['langid']} {perf_dict['cld2']}")
     
