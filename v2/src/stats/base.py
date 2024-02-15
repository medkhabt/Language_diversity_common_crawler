from __future__ import annotations
from abc import ABC, abstractmethod
from typing import Any, Optional

class Base(ABC): 
    @abstractmethod
    def generate(self, input: dict, output:dict ) -> None: 
        pass 
    
    @abstractmethod
    def counter_reset(self) -> None: 
        pass 
    @abstractmethod
    def format(self) -> str: 
        pass 
