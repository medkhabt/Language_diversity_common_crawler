
- script working with testextcat cpp implementation. 
I compiled the code and got the necessary files to predict the language of the text. ( executable + files in commands/.libs + conf file fpdb.conf) and also i had to specify the path to some language files used before compilation in the source code lib.

- made a script "lang_id.sh" to use it in the python script. 

- there is some post-proceessing of the results of the testcat executable so i get the language abbreviation without the other parts of the result. 
# TODO 
- [ ] Map the lang abbreviation of testtextcat to the WET file language param.
- [ ] get the graph of instances per language for each language model 
- [ ] get the graph of instances per language for the manual  curl.
- [ ] get the diff btw manual curl and WET and look at the common features.
- [ ] get the language detection ratio comparaison with and without the boilerplate removal 


# PROBLEMS 
- not optimized, take a lot of time to process.

