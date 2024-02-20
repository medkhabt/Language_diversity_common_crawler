```python
import logging
from fastwarc.warc import ArchiveIterator
from fastwarc.stream_io import GZipStream
import requests
import io
import configparser

from handlers.decoding_handler import DecodingHandler
from handlers.boiler_plate_handler import BoilerPlateHandler
from handlers.extraction_handler import ExtractionHandler
from handlers.language_identification_handler import LanguageIdentificationHandler
from handlers.stats_handler import StatsHandler
from handlers.repo_handler import RepoHandler

class CompareLanguageIdentificationModelsPipeline:
    """Pipeline for comparing language identification models."""
    
    def __init__(self):
        """Initialize the pipeline with necessary handlers."""
        self._decoding = DecodingHandler()
        self._boilerplate = BoilerPlateHandler()
        self._extraction = ExtractionHandler()
        self._language_identification = LanguageIdentificationHandler()
        self._stats = StatsHandler()
        self._repo = RepoHandler(100)
        
        self.start(self._decoding)
        self._clean = False
        self._decoding.set_next(self._boilerplate).set_next(self._extraction).set_next(self._language_identification).set_next(self._stats).set_next(self._repo)

    def run(self, seg_number: str, record, language_models, perf=0):
        """
        Run the pipeline for a given segment number, record, and language models.
        Parameters: 
        - seg_number: string 
        	the segement number for the traited records.
        - record : fastwarc.warc.WarcRecord
        	warc record
        - language_models: list
        	language identification models to be used in the pipline
        - perf : int 
        	0 : no perf stats ( and less executions ) 
        	1 : perf stats 
        	"""
        # @TODO For performance tests, the language identifications are not dynamic. Change that
        perf_dict = {'perf': perf, 'detect_fast': 0, 'langid': 0, 'cld2': 0}
        request = {'seg_number': seg_number, 'record': record, 'language_models': language_models, 'perf_dic': perf_dict}
        if not self._clean:
            self._repo.clean(request['seg_number'])
            self._clean = True
        self._start.handle(request)

    def start(self, start_handler):
        """Set the start handler of the pipeline."""
        self._start = start_handler

    def end(self, seg_number: str):
        """Set the end handler of the pipeline."""
        self._start = self._repo
        request = {'seg_number': seg_number, 'end': True}
        self._start.handle(request)

def pre_traitement_seg_data(res):
    """Perform pre-treatment on segment data."""
    return GZipStream(io.BytesIO(res.content))

def config():
    """Read configurations from default.ini file."""
    configs = {}
    supported_languages = ['detect_fast', 'langid', 'cld2']
    config = configparser.ConfigParser()
    config.read('default.ini')
    for key in config['MAIN']:
        configs[key] = config['MAIN'][key]
    languages = []
    for key in config['LANGUAGES']:
        languages.append(key)
    configs['languages'] = languages
    logging.info(f"The configs are {configs}")
    return configs

if __name__ == "__main__":
    configs = config()
    perf_calc = int(configs['perf'])
    warc_url = f'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/warc/CC-MAIN-20230921073711-20230921103711-{configs["segment"]}.warc.gz'
    bundle = []
    res = requests.get(warc_url)
    counter = 0
    enc_pr_ctr = 0
    dataset = []
    size = int(configs['size'])

    language_identification_models = configs['languages']
    compare_lang_pipe = CompareLanguageIdentificationModelsPipeline()
    if res.status_code == 200:
        print(f'Downloaded: {str(warc_url)}')
        for record in ArchiveIterator(pre_traitement_seg_data(res)):
            if size >= 0 and counter >= size:
                break
            compare_lang_pipe.run(configs["segment"], record, language_identification_models, perf_calc)
            counter += 1
        compare_lang_pipe.end(configs["segment"])

    else:
        print("Failed")
```
