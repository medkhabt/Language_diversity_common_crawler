from __future__ import annotations
from abc import ABC, abstractmethod
from typing import Any, Optional

class Handler(ABC): 
    @abstractmethod
    def set_next(self, handler : Handler) -> Handler: 
        """Set the handler that comes after the current handler. """
        pass 
    
    @abstractmethod
    def handle(self, request) -> Optional[Any]:
        """handle the request"""
        pass 


class AbstractHandler(Handler): 
    """ 
    Attributes
    ---------- 
    _next_handler: Handler 
    		Save the instance of the next handler in the chain.
    _instances_counter : int 
		Count the number of instances traited by the Handler.
    """ 
    _next_handler: Handler = None 
    _instances_counter : int = 0 
    
    def set_next(self, handler : Handler) -> Handler: 
        self._next_handler = handler 
        return handler;

    @abstractmethod
    def handle(self, request:Any) -> Optional[Any] : 
        """The default behavior of handler after handling the request"""
        if self._next_handler: 
            return self._next_handler.handle(request)
        else: 
            return request
        return None
    def get_number_instances_traited(self): 
        """Get the number of traited instances with the current Handler"""
        return self._instances_counter;
