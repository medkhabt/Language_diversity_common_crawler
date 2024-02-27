from __future__ import annotations
from abc import ABC, abstractmethod

class Base:
    """Abstract base class for defining Language identification functionality."""

    @abstractmethod
    def identify(self, content: str) -> Any:
        """
        Identify the given content's language .

        Args:
            content (str): The content to be identified.

        Returns:
            Any: The result of the identification.
        """
        pass
