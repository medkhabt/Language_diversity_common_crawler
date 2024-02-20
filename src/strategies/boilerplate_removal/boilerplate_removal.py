from __future__ import annotations
from abc import ABC, abstractmethod
class BoilerPlateRemoval(ABC): 
    @abstractmethod
    def apply(self, content: str) -> str: 
        pass 
 
