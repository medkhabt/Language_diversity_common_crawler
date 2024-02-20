from __future__ import annotations
from abc import ABC, abstractmethod
from typing import Any, Optional

class Handler(ABC): 
    @abstractmethod
    def set_next(self, handler : Handler) -> Handler: 
        pass 
    
    @abstractmethod
#TODO what is the type of our request ? 
    def handle(self, request) -> Optional[Any]:
        pass 


class AbstractHandler(Handler): 
    _next_handler: Handler = None 
    _instances_counter : int = 0 
    
    def set_next(self, handler : Handler) -> Handler: 
        self._next_handler = handler 
        return handler;

    @abstractmethod
    def handle(self, request:Any) -> Optional[Any] : 
        if self._next_handler: 
            return self._next_handler.handle(request)
        return None
    def get_number_instances_traited(self): 
        return self._instances_counter;
