from stats.base import Base

class Accuracy(Base):
    """Class for calculating accuracy metrics."""

## TODO this stat is heavily dependent on a fixed set of language identification models. We need to make it extendable and flexible. 
    def generate(self, input_dict: dict, output_dict: dict) -> None:
        """
        Generate accuracy metrics based on input and output dictionaries.

        Args:
            input_dict (dict): Input dictionary containing language predictions from different models.
            output_dict (dict): Output dictionary to store accuracy metrics.

        Returns:
            None
        """
        if (input_dict['detect_fast']['lang'] == input_dict['langid']['lang'] and input_dict['detect_fast']['lang'] != input_dict['cld2']['lang']):
            output_dict['cld2']['wrong'] = output_dict['cld2']['wrong'] + 1
        elif (input_dict['detect_fast']['lang'] == input_dict['cld2']['lang'] and input_dict['detect_fast']['lang'] != input_dict['langid']['lang']):
            output_dict['langid']['wrong'] = output_dict['langid']['wrong'] + 1
        elif (input_dict['langid']['lang'] == input_dict['cld2']['lang'] and input_dict['detect_fast'] != input_dict['langid']['lang']):
            output_dict['detect_fast']['wrong'] = output_dict['detect_fast']['wrong'] + 1
        if (input_dict['detect_fast']['lang'] != 'un' and input_dict['detect_fast']['lang'] != 'en' and input_dict['langid']['lang'] == 'en' and input_dict['cld2']['lang'] == 'un'):
            output_dict['detect_fast']['uniq'] = output_dict['detect_fast']['uniq'] + 1
        elif (input_dict['detect_fast']['lang'] == 'un' and input_dict['langid']['lang'] != 'en' and input_dict['cld2']['lang'] == 'un'):
            output_dict['langid']['uniq'] = output_dict['langid']['uniq'] + 1
        elif (input_dict['detect_fast']['lang'] == 'un' and input_dict['langid']['lang'] == 'en' and input_dict['cld2']['lang'] != 'un' and input_dict['cld2']['lang'] != 'en'):
            output_dict['cld2']['uniq'] = output_dict['cld2']['uniq'] + 1
        if ((input_dict['detect_fast']['lang'] == input_dict['langid']['lang'] and input_dict['langid']['lang'] == input_dict['cld2']['lang']) or (input_dict['detect_fast']['lang'] == 'un' and input_dict['langid']['lang'] == 'en' and input_dict['cld2']['lang'] == 'un')):
            output_dict['match'] = output_dict['match'] + 1

    def counter_reset(self) -> None:
        """
        Reset counters.

        Returns:
            dict: Dictionary with reset counters.
        """
        return {'uniq': 0, 'wrong': 0}

    def format(self, counter, size) -> str:
        """
        Format accuracy metrics.

        Args:
            counter (dict): Counter dictionary containing counts of unique and wrong predictions.
            size (int): Total size of predictions.

        Returns:
            str: Formatted accuracy metrics string.
        """
        return f"amount of different prediction than the other two language models\n{counter['detect_fast']['wrong'] * 100 / size}% {counter['langid']['wrong'] * 100 / size}% {counter['cld2']['wrong'] * 100 / size}%\namount of prediction with unknown prediction in the other two models.\n{counter['detect_fast']['uniq'] * 100 / size}% {counter['langid']['uniq'] * 100 / size}% {counter['cld2']['uniq'] * 100 / size}%\nthe number of perfect matches are: {counter['match'] * 100 / size}%"
