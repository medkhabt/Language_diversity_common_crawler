# Second Version 

To generate graphs from a dem file and dat file we use 'commands/fasrawc_generate_graph_comparaison.sh', there is a split to the data that we have as there are many langauges that are detected so we need to split them into multiple graphs for visibility purposes 

There is only one script for the generation of the file with this format : META_TAG HEADER_RESPONSE LI_MODEL1 .. LI_MODELN 

spaces work the best for the generation of the graphs later on. 

You will see 3 python scripts that represnt 3 implementation of the same functionnality: 

`cc_fastwarc.py`
`concurrency_cc_fastwarc.py`
`sequential_cc_fastwarc.py`

it's an experiment to test which implemenation is faster. For the sequential is the fastest. I will need to work on this more.

the second bash file is `commands/count_fastwarc_result.sh` which counts the number of occurences of a language in the each model. the output is used in the script that was first mentionned `commands/fasrawc_generate_graph_comparaison.sh`

the generated graphs are in png format in the graphs/images/{name_of_dat_file_used}

# First Version 
- script working with testextcat cpp implementation. 
I compiled the code and got the necessary files to predict the language of the text. ( executable + files in commands/.libs + conf file fpdb.conf) and also i had to specify the path to some language files used before compilation in the source code lib.

- made a script "lang_id.sh" to use it in the python script. 

- there is some post-proceessing of the results of the testcat executable so i get the language abbreviation without the other parts of the result. 

- I had to change a file containing some constants before compiling the code of langlass language identification executable.

- The language name results generated by the language identification library "libxttextcat" is based on BCP 47 codes  [the unicode translations](https://unicode.org/udhr/translations.html), there is some inconsistencies from what i noticed, for example "grc" exists in 'ISO 639-2) and not in the BCP47, while "buc" exists in "BCP47" but not in the "ISO 639-2", maybe they fill the language that do not exists in the ISO standard with the ones in BCP47 or the other way around. 
## Boilerplate 
- I used web2text lib, that uses scala, but i had to downgrade to a version scala 1.3.3 and also switch my java version to 8. 

# TODO 
- [ ] Compute graphs based on the size and language detected by cc.
- [ ] Map the lang abbreviation of testtextcat to the WET file language param. Not just with the wet files but with other language identification models so we can compare them. 
- [ ] Abstract the language identification script, there are some parts that are repetetive. 
- [X] Make sure to filter out the urls that can't be manually crawled.
- [X] Take in consideration the pages that are empty after boilerplate removal. the empty urls are removed from the stats, when comparing the cc to the locally curled result.
- [X] get the graph of instances per language for each language model 
- [X] get the graph of instances per language for the manual  curl.
- [ ] Case were meta or http header info has '|' should be traited.
- [ ] get the diff btw manual curl and WET and look at the common features.
- [ ] get the language detection ratio comparaison with and without the boilerplate removal 
- [ ] Check the amount of links that are no longer available.
- [ ] Place the script.py in the command directory
- [ ] Do the check of texts that their language can't be detected by the language identification of 'whatlang'


# PROBLEMS 
- not optimized, take a lot of time to process.

- Languages that are not part of the UTF-8 encoding. ( at least i got an error for the language identification model whatlang `Error { kind: InvalidData, message: "stream did not contain valid UTF-8" }`

- Website with small paragraph, are considered entierly boilerplate, maybe in that case i should try to identify the language with boilerpalte removal for stats. 
for example some russian websites have 'charset=windows-1251' so i had to check which charset is encoded in and translate it to utf-8 charset. 

- There is a problem with the name 'Russian' in russian in the graph, i had to change it to the english version.

- what language does 'plt' represent in langclass ? TODO: I removed the plt from the list of languages detected until i can find a fix to this. Same with "rue", "shs"

- take caring of whitespaces when it comes to language identification to make it easier to compare btwe the language identifiation models 

