
- script working with testextcat cpp implementation. 
I compiled the code and got the necessary files to predict the language of the text. ( executable + files in commands/.libs + conf file fpdb.conf) and also i had to specify the path to some language files used before compilation in the source code lib.

- made a script "lang_id.sh" to use it in the python script. 

- there is some post-proceessing of the results of the testcat executable so i get the language abbreviation without the other parts of the result. 

## Boilerplate 
- I used web2text lib, that uses scala, but i had to downgrade to a version scala 1.3.3 and also switch my java version to 8. 

# TODO 
- [ ] Map the lang abbreviation of testtextcat to the WET file language param.
- [X] get the graph of instances per language for each language model 
- [ ] get the graph of instances per language for the manual  curl.
- [ ] get the diff btw manual curl and WET and look at the common features.
- [ ] get the language detection ratio comparaison with and without the boilerplate removal 


# PROBLEMS 
- not optimized, take a lot of time to process.

- There was a problem with the encoding, that is not showing properly during the language identification step due to different encodings available on curled websites. 

- Handle the exception when the website is no longer available.

- Website with small paragraph, are considered entierly boilerplate, maybe in that case i should try to identify the language with boilerpalte removal for stats. 
for example some russian websites have 'charset=windows-1251' so i had to check which charset is encoded in and translate it to utf-8 charset. 


