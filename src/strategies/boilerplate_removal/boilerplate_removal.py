from __future__ import annotations
from abc import ABC, abstractmethod

class BoilerPlateRemoval(ABC):
    """Abstract base class for defining boilerplate removal functionality."""

    @abstractmethod
    def apply(self, content: str) -> str:
        """
        Apply boilerplate removal to the input content.

        Args:
            content (str): The content from which to remove boilerplate.

        Returns:
            str: The content with boilerplate removed.
        """
        pass

