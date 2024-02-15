import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
from repos.file_repo import FileRepository

class RepoHandler(AbstractHandler): 
# TODO fix the case where we don't specify the umber of req trigger
    _requests : list = [] ;
    _n_req_trigger: int = -1 ;  
    _first_save: bool = True;
    _repo = None; 
    _force_save = False;
    _stats: dict = {}
    def __init__(self, n = -1): 
        self.n_req_trigger = n  
        self._repo = FileRepository()
    def clean(self, seg_number:str) : 
        self._repo.clean(seg_number) 
# TODO refactor this part
    def handle(self, request:Any) -> Optional[Any] :
        logging.info("handling the repo phase")
        if('end' not in request or not request['end']): 
            self._force_save = False 
            self._instances_counter = self._instances_counter + 1
            self.save_request(request) 
        else: 
            self._force_save = True
         
# check the args
# save the request in a data_strucute 
# check if the we can save in repo or not ( is it part of the Repo handler responsibility ?  
        if(self.can_save()): 
            logging.info(f"inside the can save bloc with {self.get_number_instances_traited()}")
            # Should change the end=self._force_save if there are other things that might force the save.
            self._repo.save(request['seg_number'], self._requests, self._stats, self._instances_counter , self._first_save, end=self._force_save)   
            self._requests = [] 
        return super().handle(request);
 #       else: 
            # TODO Refactor the handle of missing args for the handlers.
 #           raise Exception("Couldn't find one/some/all of the following keys ['language_models'] in the request dict") 
    def save_request(self, request): 
        self._requests.append(request) 
        self._stats = request['stats'] 
    def can_save(self): 
        return self._force_save or ( self.n_req_trigger >= 0 ) and ( len(self._requests) == self.n_req_trigger )  

