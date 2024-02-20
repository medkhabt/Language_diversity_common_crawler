```python
from stats.base import Base

class Unknown(Base):
    """Class for calculating the count of prediction resulting in unknown."""

    def generate(self, input_dict: dict, output_dict: dict) -> None:
        """
        Generate the counts of prediction with unknowns based on input and output dictionaries.

        Args:
            input_dict (dict): Input dictionary containing language models and language predictions.
            output_dict (dict): Output dictionary to store counts of failed predictions (unknown).

        Returns:
            None
        """
        for lang in input_dict['language_models']:
            if input_dict[lang]['lang'] == 'un':
                output_dict[lang] = output_dict[lang] + 1

    def counter_reset(self) -> None:
        """
        Reset counter.

        Returns:
            int: Reset counter value.
        """
        return 0

    def format(self, counter, size) -> str:
        """
        Format unknown language counts.

        Args:
            counter (int): Counter value containing counts of the failed predictions of languages.
            size (int): Total size of predictions.

        Returns:
            str: Formatted language prediction counts that failed string.
        """
        return f"{counter['detect_fast']*100/size}% {counter['langid']*100/size}% {counter['cld2']*100/size}%\n"
```
