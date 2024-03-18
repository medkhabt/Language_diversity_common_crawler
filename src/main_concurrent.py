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
from handlers.content_length_handler import ContentLengthHandler 
from handlers.stats_handler import StatsHandler
from handlers.repo_handler import RepoHandler
from handlers.curl_handler import CurlHandler


import concurrent.futures 
from concurrent.futures import as_completed, wait
import multiprocessing
import pebble

#from memory_profiler import profile
#from guppy import hpy
import gc
#from pympler.tracker import SummaryTracker
import tracemalloc

import mem_profiling

class CompareCommonCrawlToLocalCrawlPipeline:
    def __init__(self, stats, headers, proxies): 

        self._decoding = DecodingHandler()
        self._boilerplate = BoilerPlateHandler()
        self._extraction = ExtractionHandler()
        self._content_length = ContentLengthHandler()
        self._language_identification = LanguageIdentificationHandler()
        self._stats = StatsHandler(stats)
        self._repo = RepoHandler(100)
        self._local = CurlHandler(headers, proxies)

        self._clean = False
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

        perf_dict = {'perf': perf, 'detect_fast': 0, 'langid': 0, 'cld2': 0}
        request = {'seg_number': seg_number, 'record': record, 'language_models': language_models, 'type-content' : 'warc', 'perf_dic': perf_dict, 'format' : []}
        request['format'].extend(['meta_warc', 'http_header_warc', 'length_warc'])
        for lang in language_models:
            request['format'].append(lang + '_warc')
        request['format'].extend(['meta_curl', 'http_header_curl', 'length_curl'])
        for lang in language_models:
            request['format'].append(lang + '_curl')
        request['format'].append('redirect')
## First phase
        self._decoding.set_next(self._extraction)
        if not self._clean:
            self._repo.clean(request['seg_number'])
            self._clean = True
        result_first_phase = self._decoding.handle(request)
        if(type(result_first_phase) == int):
            return 1
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
                if key == 'seg_number' or key == 'format' or key == 'redirect':
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
                if key == 'seg_number' or key == 'format' or key == 'redirect':
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
            self._repo.handle(response)
            #response.clear()
            #del response['content_curl']
            #del response['content_warc']
    def logs_from_content_pipe(self, request): 
# boilerplate -> content_length -> li : second pipe 
        if (type(request) == int and request == 1): 
            return 1
        self._extraction.set_next(self._boilerplate).set_next(self._content_length).set_next(self._language_identification) 
        return self._extraction.handle(request); 
    def get_content_locally_pipe(self, request): 
       return self.logs_from_content_pipe(self._local.handle(request))

## Third phase 
### save the state, and in case the conditions are met save the info in a file.
   
    def end(self, seg_number: str, language_models):
        """Set the end handler of the pipeline."""
        request = {'seg_number': seg_number, 'format' : [] , 'end': True}
        request['format'].extend(['meta_warc', 'http_header_warc', 'length_warc'])
        for lang in language_models:
            request['format'].append(lang + '_warc')
        request['format'].extend(['meta_curl', 'http_header_curl', 'length_curl'])
        for lang in language_models:
            request['format'].append(lang + '_curl')

        request['format'].append('redirect')
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
        self._decoding.set_next(self._extraction).set_next(self._boilerplate).set_next(self._language_identification).set_next(self._stats)

    def run(self, seg_number: str, record, language_models, perf=0):
        """
        Run the pipeline for a given segment number, record, and language models.
        Parameters: 
        - seg_number: string 
        	the segement number for the traited records.
        - record : fastwarc.warc.WarcRecord
        	warc record
        - language_models: list
        	language identification models to be used in the pipeline
        - perf : int 
        	0 : no perf stats ( and less executions ) 
        	1 : perf stats 
        	"""
        # @TODO For performance tests, the language identifications are not dynamic. Change that
        perf_dict = {'perf': perf, 'detect_fast': 0, 'langid': 0, 'cld2': 0}
        request = {'seg_number': seg_number,'type-content' : 'warc', 'record': record, 'language_models': language_models, 'perf_dic': perf_dict, 'format' : ['meta', 'http_header', 'detect_fast', 'langid', 'cld2']}

        if not self._clean:
            self._repo.clean(request['seg_number'])
            self._clean = True
        response = self._start.handle(request)
        # pre traitement of the pipeline result before saving it.
        for lang in language_models: 
           response[lang] = response[lang]['lang']
        self._repo.handle(response)
 
    def start(self, start_handler):
        """Set the start handler of the pipeline."""
        self._start = start_handler

    def end(self, seg_number: str, language_models):
        """Set the end handler of the pipeline."""
        self._start = self._repo
        request = {'seg_number': seg_number, 'format' : ['meta', 'http_header', 'detect_fast', 'langid', 'cld2'] , 'end': True}
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

def main(): 
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
        #with concurrent.futures.ThreadPoolExecutor(max_workers=100) as executor: 
        with pebble.ThreadPool(max_workers=200, max_tasks=21) as executor: 
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
if __name__ == "__main__":
    #hp = hpy()
    #tracemalloc.start(10)
    main()
    #h = hp.heap()
    #snapshot = tracemalloc.take_snapshot()
    #mem_profiling.top_n(25, snapshot, 'test')
    #top_stats = snapshot.statistics('lineno')
    #print("**** tracemalloc stats: ") 
    #for stat in top_stats[:10]: 
    #    print(stat)
'''
    print(f"^^^^^^^^^^^^^^^^^^^^^^^^^ h is {h}")
    print(f"^^^^^^^^^^^^^^^^^^^^^^^^^ h by type {h.bytype}")
    print(f"^^^^^^^^^^^^^^^^^^^^^^^^^ h by rcs {h.byrcs}")
    byrcs = h.bytype


    print(f"****** check the byrcs line that was chosen {byrcs[0]} ")
    print(f"****** we are now bytes by rc {byrcs[0].byrcs} ")
    print(f"****** the str size  {byrcs[0].byrcs[0].bysize} ")

    print(f"******  we are in the byrc by id {byrcs[0].byid} ")
 
    print(f"******  the rcs 1 -> byvia {byrcs[0].byvia} ")
    cleaning_part = byrcs[0].byvia[0].referrers
  
    print(f"******  cleaning part : {cleaning_part} ")
    print(f"******  cleaning part by rcs {cleaning_part.byrcs}")

    t = byrcs[1].referrers.byrcs 
    print(f"****** referres : {t} ")
    print(f"****** domisize : {t.domisize} ")
    print(f"****** domisize of the cleaning part : {cleaning_part.byrcs.domisize} ")
'''
