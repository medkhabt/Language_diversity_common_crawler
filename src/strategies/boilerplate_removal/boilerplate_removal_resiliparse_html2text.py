from strategies.boilerplate_removal.boilerplate_removal import BoilerPlateRemoval 
from resiliparse.extract.html2text import extract_plain_text
class ResiliParseHtml2Text(BoilerPlateRemoval):
    def apply(self, content: str) -> str: 
        return extract_plain_text(content, main_content=True); 
