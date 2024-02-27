from __future__ import annotations
from abc import ABC, abstractmethod
from typing import Any, Optional

class Base(ABC): 
    """Abstract base class for defining a base interface."""
    @abstractmethod
    def generate(self, input: dict, output:dict ) -> None: 
        """
        Generate output based on input.

        Args:
            input (dict): The input dictionary.
            output (dict): The output dictionary.

        Returns:
            None
        """
        pass 
    
    @abstractmethod
    def counter_reset(self) -> None: 
        """
        Reset counters or internal state.

        Returns:
            None
        """
        pass 
    @abstractmethod
    def format(self) -> str:
        """
        Format the output as a string.

        Returns:
            str: The formatted output.
        """ 
        pass 
