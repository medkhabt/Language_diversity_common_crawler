import os 
import logging
class FileRepository: 
    def clean(self, seg_number:str) : 
    """Clean the log file associated for the seg_number"""
        os.system(f'rm logs/{seg_number}.log 2>/dev/null');
    def save(self, seg_number, requests, stats, size, first_save=False, end=False):
    """
	Save the requests in the right format in the associated files.

	Parameters
	----------
	seg_number : int 
		the seg_number associated to the requests
	requests : list 
		list of the requests to be saved in a file.
	stats : dict 
		dict of the stats {'counter': dic , 'instance": stats.base.Base} that should be used to fill the stats files. 
	size : int 
		size of the requests list
        first_save : bool
		Is it the first save in a file for the FileRepository instance	
	end : bool 
		Is it the last request to handle for the current pipeline.
    """
	#TODO all of this part is hardcoded for now, in case we want to experiment with other language identfication models we need to refactor this for a smoother experience :D 
        if(first_save): 
            with open(f'logs/{seg_number}.log', 'w', encoding='utf-8') as f: 
                f.write(f"meta|http_header|detectfast|langid|cld2|pre-detectfast|pre-langid|pre-cld2\n")
        with open(f'logs/{seg_number}.log', 'a', encoding='utf-8') as f: 
	# Found a problem with the max caraters allowed in a single line, the process get killed.
    #        json.dump(dataset, f, ensure_ascii = False, indent=2)
            for dr in requests:
                f.write(f"{dr['meta']}|{dr['http_header']}|{dr['detect_fast']['lang']}|{dr['langid']['lang']}|{dr['cld2']['lang']}|{dr['detect_fast']['precision']}|{dr['langid']['precision']}|{dr['cld2']['precision']}\n")
            if(end): 
                for stat in stats:  
                    with open(f'logs/{seg_number}_{stat}.log', 'w', encoding='utf-8') as f: 
                        f.write(stats[stat]['instance'].format(stats[stat]['counter'], size))

# TODO with saving data before traversing ever instance we need to be careful with the stats.

#        with open(f'../logs/test_refactor{seg_number}_performance', 'w', encoding='utf-8') as f: 
#            f.write(f"{perf_dict['detect_fast']} {perf_dict['langid']} {perf_dict['cld2']}")
     
