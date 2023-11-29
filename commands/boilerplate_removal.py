from __future__ import absolute_import
from __future__ import division, print_function, unicode_literals

import requests
import requests.exceptions
import justext
import sys

# TODO Remove this import
import os


def main():
    try:
        original_page = requests.get(str(sys.argv[1]))
        page = original_page.text.encode('utf-8')	
        any_lang_stop_words = get_all_stop_words()
	
	## all what comes before this will be removed 
        paragraphs = justext.justext(page, any_lang_stop_words, 99, 100, 0.1, 0.32, 0.2, 200, True)
        for paragraph in paragraphs:
            if not (paragraph['class'] == 'bad') :
                print("".join(paragraph['text_nodes']))

    except requests.exceptions.InvalidSchema as e: 
        print("invalid url") 
        return 0


def get_all_stop_words():
    stop_words = set()
    for language in justext.get_stoplists():
        stop_words.update(justext.get_stoplist(language))

    return stop_words


if __name__ == "__main__":
    exit(main())
