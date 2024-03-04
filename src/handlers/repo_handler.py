import logging
from handlers.handler_basis import AbstractHandler
from typing import Any, Optional
from repos.file_repo import FileRepository

class RepoHandler(AbstractHandler): 
    """ 
    Attributes
    ---------- 
    _requests: list 
  	List of stored batch requests before getting saved. 
    _n_req_trigger: int
	Minimum number of requests before saving them in the proper resource. 
    _first_save: bool
	Is it the first save done by the Repo Handler.
    _repo: repose.file_repo.FileRepository ; 
        the implementation of the repo ( for now we only have the file implementation )
    _force_save: bool
	Force a save even tho all conditions are not met.
    _stats: dict 
	Contains the stats that we're generated If there was a StatsHandler in the pipleline.	
    """ 
# TODO fix the case where we don't specify the umber of req trigger
    _requests : list = [] ;
    _n_req_trigger: int = -1 ;  
    _first_save: bool
    _repo = None; 
    _force_save = False;
    _stats: dict = {}
    def __init__(self, n = -1): 
        """
	Initialize the Attributes.
	Parameters 
	---------
      	n : int 
	    Minimum number of requests before saving them in the proper resource. 
        """ 
        self._first_save = True
        self.n_req_trigger = n  
        self._repo = FileRepository()
    def clean(self, seg_number:str) : 
        """ Call the clean method from the repo implementation"""
        self._repo.clean(seg_number) 
    def handle(self, request:Any) -> Optional[Any] :
        """ Handle the storage of the requests in the instance, saving the requests in the resource specified in the repo implementation and handle the end of a pipepline (no more records) """
        logging.info(f"handling the repo phase with requests {self._requests} with size {len(self._requests)}")
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
            self._repo.save(request['seg_number'], self._requests, request['format'], self._stats, self._instances_counter , self._first_save, end=self._force_save)   
            if(self._first_save): 
                self._first_save = False
            self._requests = [] 
        return super().handle(request);
 #       else: 
            # TODO Refactor the handle of missing args for the handlers.
 #           raise Exception("Couldn't find one/some/all of the following keys ['language_models'] in the request dict") 
    def save_request(self, request): 
        """ Store the request in the instance with its stats. """
        self._requests.append(request) 
        self._stats = request['stats'] 
    def can_save(self): 
        """ Check if the instance is in a state of saving the instance in the repo ressource.
       """
        return self._force_save or ( self.n_req_trigger >= 0 ) and ( len(self._requests) == self.n_req_trigger )  

