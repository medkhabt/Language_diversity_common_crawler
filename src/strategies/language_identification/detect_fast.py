from strategies.language_identification.base import Base
from resiliparse.parse.lang import detect_fast as d
import time

class DetectFast(Base):
    """Class for language identification using Detect Fast."""

    def identify(self, content: str, perf_dic: dict) -> dict:
        """
        Identify the language of the given content using Detect Fast.

        Args:
            content (str): The content to be identified.
            perf_dic (dict): Performance dictionary to store performance metrics.

        Returns:
            dict: The language identification result.
        """
        if perf_dic['perf'] == 1:
            start = time.process_time()
            for i in range(100):
                d(content)[0]
            duration = time.process_time() - start
            perf_dic['detect_fast'] = duration
        return {'lang': d(content)[0], 'precision': d(content)[1]}
