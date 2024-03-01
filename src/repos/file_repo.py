import os 
import logging
class FileRepository: 
    def clean(self, seg_number:str): 
        """Clean the log file associated for the seg_number"""
        os.system(f'rm logs/{seg_number}.log 2>/dev/null');
    def save(self, seg_number, requests, save_format, stats, size, first_save=False, end=False):
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
        if(first_save): 
            with open(f'logs/{seg_number}.log', 'w', encoding='utf-8') as f: 
                f.write(f"{'|'.join(save_format)}\n")
        with open(f'logs/{seg_number}.log', 'a', encoding='utf-8') as f: 
	# Found a problem with the max caraters allowed in a single line, the process get killed.
    #        json.dump(dataset, f, ensure_ascii = False, indent=2)
            for dr in requests:
                line_arr = []
                for key in save_format: 
                    line_arr.append(dr[key])
                #print(f"the line arr before joining is {line_arr} ")
                f.write(f"{'|'.join(line_arr)}\n")
                #print(f"the stats are {stats}")
## TODO fix the stats and uncomment the stats.
#            if(end): 
                #for stat in stats:  
                #    with open(f'logs/{seg_number}_{stat}.log', 'w', encoding='utf-8') as f: 
                #        f.write(stats[stat]['instance'].format(stats[stat]['counter'], size))

# TODO with saving data before traversing ever instance we need to be careful with the stats.

#        with open(f'../logs/test_refactor{seg_number}_performance', 'w', encoding='utf-8') as f: 
#            f.write(f"{perf_dict['detect_fast']} {perf_dict['langid']} {perf_dict['cld2']}")
     
