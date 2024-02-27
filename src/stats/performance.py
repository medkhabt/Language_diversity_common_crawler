from stats.base import Base
import logging

class Performance(Base):
    """Class for calculating performance metrics."""

    def generate(self, input_dict: dict, output_dict: dict) -> None:
        """
        Generate performance metrics based on input and output dictionaries.

        Args:
            input_dict (dict): Input dictionary containing language models and their time of execution.
            output_dict (dict): Output dictionary to store performance metrics.

        Returns:
            None
        """
        for lang in input_dict['language_models']:
            output_dict[lang] = output_dict[lang] + input_dict['perf_dic'][lang]

    def counter_reset(self) -> None:
        """
        Reset counter.

        Returns:
            int: Reset counter value.
        """
        return 0

    def format(self, counter, size) -> str:
        """
        Format performance metrics.

        Args:
            counter (int): Counter value containing performance metrics.
            size (int): Total size of requests.

        Returns:
            str: Formatted performance metrics string.
        """
        return f"{counter['detect_fast']/size} {counter['langid']/size} {counter['cld2']/size} \n"

