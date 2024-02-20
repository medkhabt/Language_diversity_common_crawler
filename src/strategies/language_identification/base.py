from __future__ import annotations
from abc import ABC, abstractmethod
from typing import Any, Optional

class Base:
    @abstractmethod
    def identify(self, content:str): 
        pass 
    
