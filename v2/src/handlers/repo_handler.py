import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
class RepoHandler(AbstractHandler): 
# TODO fix the case where we don't specify the umber of req trigger
    _requests : list = [] ;
    _n_req_trigger: int = -1 ;  
    def __init__(self, n = -1): 
        self.n_req_trigger = n 
    def handle(self, request:Any) -> Optional[Any] :
        log.info("handling the repo phase")
# check the args
# save the request in a data_strucute 
# check if the we can save in repo or not ( is it part of the Repo handler responsibility ?  
        self.save_request(request) 
        if(self.can_save_in_file()): 
            self.save_in_file()   
            self.requests = [] 
        return super().handle(request);
 #       else: 
            # TODO Refactor the handle of missing args for the handlers.
 #           raise Exception("Couldn't find one/some/all of the following keys ['language_models'] in the request dict") 
    def save_request(self, request): 
        self._requests.append(request) 
    def can_save_in_file(self): 
        return ( self.n_req_trigger >= 0 ) and ( self.get_number_instances_traited() == self.n_req_trigger )  

    def save_in_file():
    

	#TODO all of this part is hardcoded for now, in case we want to experiment with other language identfication models we need to refactor this for a smoother experience :D 
        with open(f'../logs/{seg_number}.log', 'w', encoding='utf-8') as f: 
	# Found a problem with the max caraters allowed in a single line, the process get killed.
    #        json.dump(dataset, f, ensure_ascii = False, indent=2)
            f.write(f"meta|http_header|detectfast|langid|cld2|pre-detectfast|pre-langid|pre-cld2\n")
            for dr in self_requests:
                f.write(f"{dr['meta']}|{dr['http_header']}|{dr['detect_fast']['lang']}|{dr['langid']['lang']}|{dr['cld2']['lang']}|{dr['detect_fast']['precision']}|{dr['langid']['precision']}|{dr['cld2']['precision']}\n")
        with open(f'logs/{seg_number}_accuracy.log', 'w', encoding='utf-8') as f: 
            f.write("amount of different prediction than the other two language models\n")
            f.write(f"{accuracy['detect_fast']['wrong'] * 100/accuracy['size']}% {accuracy['langid']['wrong'] * 100 / accuracy['size']}% {accuracy['cld2']['wrong']* 100 / accuracy['size']}%\n")
            f.write("amount of prediction with unknown prediction in the other two models.\n")
            f.write(f"{accuracy['detect_fast']['uniq']* 100 / accuracy['size']}% {accuracy['langid']['uniq']* 100 / accuracy['size']}% {accuracy['cld2']['uniq']* 100 / accuracy['size']}%\n")
	
            f.write(f"the number of perfect matches are: {accuracy['match']*100/accuracy['size']}%")

        with open(f'logs/{seg_number}_unknowns', 'w', encoding='utf-8') as f: 
            f.write(f"{unknowns_dic['detect_fast']* 100 / accuracy['size']}% {unknowns_dic['langid']* 100 / accuracy['size']}% {unknowns_dic['cld2']* 100 / accuracy['size']}%\n")

        with open(f'logs/{seg_number}_performance', 'w', encoding='utf-8') as f: 
            f.write(f"{perf_dict['detect_fast']} {perf_dict['langid']} {perf_dict['cld2']}")
     
