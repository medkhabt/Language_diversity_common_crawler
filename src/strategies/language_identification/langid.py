from strategies.language_identification.base import Base
import langid
import time

class LangId(Base):
    """Class for language identification using LangId."""

    def identify(self, content: str, perf_dic: dict) -> dict:
        """
        Identify the language of the given content using LangId.

        Args:
            content (str): The content to be identified.
            perf_dic (dict): Performance dictionary to store performance metrics.

        Returns:
            dict: The language identification result.
        """
        if perf_dic['perf'] == 1:
            start = time.process_time()
            for i in range(100):
                langid.classify(content)[0]
            duration = time.process_time() - start
            perf_dic['langid'] = duration
        return {'lang': langid.classify(content)[0], 'precision': langid.classify(content)[1]}

