from strategies.boilerplate_removal.boilerplate_removal import BoilerPlateRemoval
from resiliparse.extract.html2text import extract_plain_text

class ResiliParseHtml2Text(BoilerPlateRemoval):
    """Class for extracting plain text from HTML content while removing boilerplate."""

    def apply(self, content: str) -> str:
        """
        Apply HTML to plain text conversion and boilerplate removal to the input content.

        Args:
            content (str): The HTML content to be converted to plain text and have boilerplate removed.

        Returns:
            str: The plain text content with boilerplate removed.
        """
        return extract_plain_text(content, main_content=True)

