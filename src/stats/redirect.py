from stats.base import Base

class Redirect(Base):
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
        if input_dict['redirect'] == 'yes':
                output_dict['base'] = output_dict['base'] + 1

    def counter_reset(self) -> None:
        """
        Reset counter.

        Returns:
            int: Reset counter value.
        """
        return {'base':0}

    def format(self, counter, size) -> str:
        """
        Format unknown language counts.

        Args:
            counter (int): Counter value containing counts of the failed predictions of languages.
            size (int): Total size of predictions.

        returns:
            str: formatted language prediction counts that failed string.
        """
        return f" the percentage of redirections is {counter['base']*100/size}%\n"
