## Introduction 

- In this repo, you will find some testing cases in the goal of having a more language diverse web crawling. 

The goal is : 

getting a file with seg urls of common crawl dump => downloading the segement => iterating over the warc records on the seg. => extracting some metadata => boilerplate removal => identifying the language => extracting the url => downloading the content of the url (with different configurations/params) => boilerplate removal => identifying the language. => Comparing the diff when it comes to lang. ident. between the commoncrawl dump and our strat. 

To do that, first of all, we need to pick a language identfication model that we are going to compare with the detected languages for the multiple cases that we are testing.  We picked models that are out of shelf. The ones that are tested for now are [Detect_fast, langid, cld2], It's also easy to add more. 

For the tests. It was based on testing 3 models.
We are testing the usability of each language identification model for our use case. 
 - effectiveness  ({name}_accurracy.log), ({name}_unknown.log)
   - How much is a language identification model identify the same as other identification model. 
   - How many instances a language identification model is identify a language when others don't. 
 - efficiency 
   Calculating the time spent for a language identification of a content. x100 to avoid micro-benchmarking. ({name}_performance.log)


 
## INSTALLATION

For the first time 
```
conda env create --file my_dependencies.yml 
conda activate lang_div
```

and 


``` 
conda env update --file my_dependencies.yml
```
if some changes were necessary in the `my_dependencies.yml` file


## Run the comparaison of the language identification models. 
There is a file `default.ini` containing the configs used for the script. 
You can specify the segement number that the experiment will be on. (a segement contains many warc records from commoncrawl) 
and execute ```make all_one_seg``` 

or you can specify `NumberOfSegs` to execute the script on multiple segements by running ```make all``` 


### The execution of the pipeline for a single segement results in : 
command `make all_one_seg`
	- logs/{SEGEMENT_NUMBER}.log  (containing the logs concerning the meta headers, http headers, and the language identifications) 
        - logs/{SEGEMENT_NUMBER}_{STAT}.log (contains the logs about the stats that are implemented in stats/* and that are handled by the `handlers/stat_handler.py::StatHandler`
        - graphs/dat/{SEGEMENT_NUMBER}.dat (contains how many times a langauge was identified by a language identification model)
        - graphs/images/{SEGEMENT_NUMBER}/* (contains histograms to compare the number of language identification for each language. Scale is logarithmic. And the graph is split into multiple subgraphs depending on the number of languages that were detected, so we don't lose the lisibility of the data on the graphs).

### The execution of the pipepline for multiple segements results in:  

command `make all`

each segement will have the same results as if we executed the pipeline just for the specific segement, and also there will be another file generated in each one of the locations specified in the upper section. Containing the date of the execution which concatinate all the logs/dats/graphs of the segements executed for this pipeline. 

### clean up 
``` make clean ```


## Architecture : 

src
├── extractions ( the data that we extract from the page content ) 
├── handlers ( Chain of responsability DP, seemed usefull for this kind│    of use case)
├── repos ( for saving the data ) 
├── stats ( for generating stats )
└── strategies ( any actions done on the response/content of the page )
    ├── boilerplate_removal
    ├── decoding
    └── language_identification

