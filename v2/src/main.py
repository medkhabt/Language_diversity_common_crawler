import logging
#WARC Extraction
from fastwarc.warc import ArchiveIterator 
from fastwarc.stream_io import GZipStream
import requests
import sys
import io
# not used yet.



from handlers.decoding_handler import DecodingHandler 
from handlers.boiler_plate_handler import BoilerPlateHandler 
from handlers.extraction_handler import ExtractionHandler 
from handlers.language_identification_handler import LanguageIdentificationHandler 
from handlers.stats_handler import StatsHandler 
from handlers.repo_handler import RepoHandler 

# Create instances of the necessary handlers for our pipeline.
# decode => boilerplate => extraction => language_identificaiton => stats => repo.
class CompareLanguageIdentificationModelsPipeline:
    def __init__(self):
        self._decoding = DecodingHandler() #if(self._decoding == None); 
        self._boilerplate = BoilerPlateHandler() #if(self._boilerplate == None); 
        self._extraction = ExtractionHandler() #if(self._extraction== None); 
        self._language_identification = LanguageIdentificationHandler() #if(self._language_identification == None); 
        self._stats = StatsHandler() #if(self._stats== None); 
        self._repo = RepoHandler(100) #if(self._repo== None); 
        
        self.start(self._decoding)
        self._clean = False;
        self._decoding.set_next(self._boilerplate).set_next(self._extraction).set_next(self._language_identification).set_next(self._stats).set_next(self._repo)
    def run(self, seg_number:str, record, language_models, perf=0):
        perf_dict = {'perf' : perf, 'detect_fast' : 0, 'langid' : 0, 'cld2' : 0}

        request = {
            'seg_number' : seg_number,
	    'record' : record, 
	    'language_models': language_models, 
	    'perf_dic' : perf_dict
        }
        self._start.handle(request)
        if (not self._clean): 
            self._repo.clean(request['seg_number']); 
            self._clean = True
    def start(self, start_handler): 
        self._start = start_handler

def pre_traitement_seg_data(res): 
    return GZipStream(io.BytesIO(res.content))

if __name__ == "__main__" : 
    #logging.getLogger().setLevel(logging.INFO)
# pre-traitement and initialization before the execution of the pipleline. 

    if len(sys.argv) > 1: 
        perf_calc = 1 if sys.argv[1] == '1' else 0;
    else: 
        perf_calc = 0 
    if len(sys.argv) > 2 and len(sys.argv[2]) == 5 : 
        seg_number = sys.argv[2]
    else: 
        seg_number = '00000'
    print(f"args are : [ perf_calc : {perf_calc} , seg_number = {seg_number} ]")
    warc_url = f'https://data.commoncrawl.org/crawl-data/CC-MAIN-2023-40/segments/1695233505362.29/warc/CC-MAIN-20230921073711-20230921103711-{seg_number}.warc.gz'
    bundle = [];
    res = requests.get(warc_url); 
    counter = 0;
    enc_pr_ctr = 0;
    dataset = []
    size = 1000
    #accuracy = {'size': 0, 'match' : 0, 'detect_fast' : {'wrong' : 0 , 'uniq' : 0}, 'langid' : {'wrong' : 0 , 'uniq' : 0}, 'cld2' : {'wrong' : 0 , 'uniq' : 0}}
    #unknowns_dic = {'detect_fast' : 0, 'langid' : 0, 'cld2' : 0}

# TODO this also shoudl be a param
    language_identification_models = ['detect_fast', 'langid', 'cld2'] 
    compare_lang_pipe = CompareLanguageIdentificationModelsPipeline()
    if res.status_code == 200: 
        print(f'Downloaded: {str(warc_url)}')
        for record in ArchiveIterator(pre_traitement_seg_data(res)):
            if counter >= size :
                break;
            compare_lang_pipe.run(seg_number, record, language_identification_models, perf_calc)
            counter += 1
# TODO Probably add a log to the urls that couldn't get decoded.
# TODO don't really need the else
    else :
        print("Failed")

## it is going to contain the argument extraction part. 


