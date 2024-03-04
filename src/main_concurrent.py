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
from handlers.curl_handler import CurlHandler


import concurrent.futures 
from concurrent.futures import as_completed, wait

class CompareCommonCrawlToLocalCrawlPipeline:
    def __init__(self, stats, headers, proxies): 
        self._decoding = DecodingHandler()
        self._boilerplate = BoilerPlateHandler()
        self._extraction = ExtractionHandler()
        self._language_identification = LanguageIdentificationHandler()
        self._stats = StatsHandler(stats)
        self._repo = RepoHandler(100)
        self._local = CurlHandler(headers, proxies)

        self._clean = False
        self._decoding
# decoding -> extractions : first pipe. 
## after getting the uri. 
# boilerplate -> li -> stats : second pipe 
# extraction -> second pipe : thrid pipe (save the requests in the pipe instead of the handler) : post traitement to feed it to the repo handler ( in that post traitement we can wait for all the pipeline in case we executed them in a concurrent way ) 
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
#request['type-content'] = 'local'
        print(" ^^^^^^^^^^^^^^starting the run of the pipeline")

        perf_dict = {'perf': perf, 'detect_fast': 0, 'langid': 0, 'cld2': 0}
        request = {'seg_number': seg_number, 'record': record, 'language_models': language_models, 'type-content' : 'warc', 'perf_dic': perf_dict, 'format' : []}
        request['format'].extend(['meta_warc', 'http_header_warc'])
        for lang in language_models:
            request['format'].append(lang + '_warc')
        request['format'].extend(['meta_curl', 'http_header_curl'])
        for lang in language_models:
            request['format'].append(lang + '_curl')
## First phase
        self._decoding.set_next(self._extraction)
        if not self._clean:
            print("--- we are cleaning")
            self._repo.clean(request['seg_number'])
            self._clean = True
        print("------------- before starting the first phase")
        result_first_phase = self._decoding.handle(request)
        #print(f"************** first phase result {result_first_phase}")
        if(type(result_second_phase) == int):
            return 1
        #print(f"response from first part is {result_first_phase}")
# we get the uri
## Second phase 
# use the uri to strat the sub_pipe that traits the content and give the necessary stats.
        result_second_phase = self.logs_from_content_pipe(result_first_phase)
        if(type(result_second_phase) == int): 
            return 1
        result = self._stats.handle(result_second_phase);
        #print(f"result local is {result_local}")
        response = {'stats' : {}}
        if type(result) is dict : 
            for key, value in result.items(): 
                if key == 'seg_number' or key == 'format':
                    response[key] = value; 
                elif key == 'stats': 
                    response['stats']['warc'] = value
                elif key in language_models:
                    response[key + '_warc'] = value['lang'] 
                else:
                    response[key + '_warc'] = value
        else : 
            response['warc'] = result
        
        result_local = self.get_content_locally_pipe(result_first_phase)
        if type(result_local) is dict : 
            for key, value in result_local.items(): 
                if key == 'seg_number' or key == 'format':
                    response[key] = value; 
                elif key in language_models:
                    response[key + '_curl'] = value['lang'] 
                elif key == 'stats': 
                    response['stats']['curl'] = value
                else: 
                    response[key + '_curl'] = value
                
        else: 
            response['curl'] = result_local
# in case we have different int returns that are valid, we should probably look at this piece of code, it will def cause bugs. 
        if('curl' not in response and 'warc' not in response) : 
            print("1") 
            self._repo.handle(response)
        else : 
            print("0")
        #print(f"response from cc part is {response}")
    def logs_from_content_pipe(self, request): 
# boilerplate -> li : second pipe 
        if (type(request) == int and request == 1): 
            return 1
        self._extraction.set_next(self._boilerplate).set_next(self._language_identification) 
        #print(f"the request inside the content pipe is {request}")
        return self._extraction.handle(request); 
    def get_content_locally_pipe(self, request): 
       return self.logs_from_content_pipe(self._local.handle(request))

## Third phase 
### save the state, and in case the conditions are met save the info in a file.
   
    def end(self, seg_number: str, language_models):
        """Set the end handler of the pipeline."""
        request = {'seg_number': seg_number, 'format' : [] , 'end': True}
        request['format'].extend(['meta_warc', 'http_header_warc'])
        for lang in language_models:
            request['format'].append(lang + '_warc')
        request['format'].extend(['meta_curl', 'http_header_curl'])
        for lang in language_models:
            request['format'].append(lang + '_curl')

        self._repo.handle(request)

class CompareLanguageIdentificationModelsPipeline:
    """Pipeline for comparing language identification models."""
    
    def __init__(self, stats):
        """Initialize the pipeline with necessary handlers."""
        self._decoding = DecodingHandler()
        self._boilerplate = BoilerPlateHandler()
        self._extraction = ExtractionHandler()
        self._language_identification = LanguageIdentificationHandler()
        self._stats = StatsHandler(stats)
        self._repo = RepoHandler(100)
        
        self.start(self._decoding)
        self._clean = False
        self._decoding.set_next(self._extraction).set_next(self._boilerplate).set_next(self._language_identification).set_next(self._stats).set_next(self._repo)

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
        request = {'seg_number': seg_number,'type-content' : 'warc', 'record': record, 'language_models': language_models, 'perf_dic': perf_dict, 'format' : ['meta_cc', 'http_header_cc', 'lang_cc', 'meta_curl', 'http_header_curl', 'lang_curl']}
#f"|http_header_warc|lang_warc|meta_curl|http_header_curl|lang_curl|redirect|uri\n" 
# {dr['meta']}|{dr['http_header']}|{dr['detect_fast']['lang']}|{dr['langid']['lang']}|{dr['cld2']['lang']}|{dr['detect_fast']['precision']}|{dr['langid']['precision']}|{dr['cld2']['precision']}\n
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

    configs['languages'] = []  
    configs['stats'] = []  
    configs['headers'] = {} 
    configs['proxies'] = {} 

    for key in config['MAIN']:
        configs[key] = config['MAIN'][key]
    for key in config['LANGUAGES']:
        configs['languages'].append(key)
    for key in config['STATS']:
        configs['stats'].append(key)
    for key in config['HEADERS']: 
        configs['headers'][key] = config['HEADERS'][key]; 
    for key in config['PROXY']:
        configs['proxies'][key] = config['PROXY'][key]
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
    if(configs['mode']=="CC_Curl"): 
        compare_lang_pipe = CompareCommonCrawlToLocalCrawlPipeline(configs['stats'], configs['headers'], configs['proxies'])
    elif(configs['mode']=="Lang_Cmp"):
        compare_lang_pipe = CompareLanguageIdentificationModelsPipeline(configs['stats'])
    else: 
        print("You didn't choose a valid pipeline")
        exit(1)
    if res.status_code == 200:
        print(f'Downloaded: {str(warc_url)}')
        with concurrent.futures.ThreadPoolExecutor(max_workers=110) as executor: 
            future_list = []
            for record in ArchiveIterator(pre_traitement_seg_data(res)):
                if size >= 0 and counter >= size:
                    break
                record.freeze()
                future_list.append(executor.submit(compare_lang_pipe.run, configs["segment"], record, language_identification_models, perf_calc ))
#                compare_lang_pipe.run(configs["segment"], record, language_identification_models, perf_calc)
                counter += 1
            done = wait(future_list)
            print("reach the end of the pipeline")
            compare_lang_pipe.end(configs["segment"], language_identification_models)
            

    else:
        print("Failed")
