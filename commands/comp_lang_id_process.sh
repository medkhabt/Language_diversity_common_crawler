#! bin/bash

## TODO ATTENTION: I STILL DIDN'T TEST THIS
## We compare the language identificator a config file. For now it is a fixed size, and the fixed size is 2 for now. 
## TODO Make it extensible (specify as much language identification model as we want.)


############## SET CONSTANTS ####################################
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );

typeset -A config 

config=(
    [lang_model_1_name]="langclass"
    [lang_model_1_exec]="lang_id.sh"
    [lang_model_2_name]="whatlang"
    [lang_model_2_exec]="lang_id_whatlang.sh"
)
# TODO should i really add the path of the exceution of the script with the language model exec path. Fleeeeeme.



############ TRAITE OPTIONS ####################################
## TODO specify a param for the lenght of the cc dump file for now. 
while getopts ":h" option;do 

   case $option in
      h) # display Help
         exit;;
      c) # generating the cc dump 
	## Need to add a arg to define the the language identification model to use.
         cd .. ; python3 script.py; cd $SCRIPT_DIR  		
   esac
done


############ READ CONFIG ####################################
while read line 
do 
    # echo the line and keeping just the lines with '=' sign
    if echo $line | grep -F = &>/dev/null
    then 
	varname=$(echo "$line" | cut -d '=' -f 1)
	config[$varname]=$(echo "$line" | cut -d '=' -f 2-)
    fi 
done < default.conf

## We have two options, one to generate the cc dump file and the other without, using just the urls that we get ? or if we already have a cc dump log for one of the language identification models .


## For now the local curl is coupled to the language identification model so we need to curl for each IL but i should separate the two. @TODO  

## TODO is there a possible way to do this in parallel.
## the default naming of ghe cc file would be cc_{lang_id_name}.log
### For the first lang. id model 
log_name_1="cc_${config['lang_model_1_name']}" 
log_name_2="cc_${config['lang_model_2_name']}" 
bash locally_curled.sh $log_name_1 ${config['lang_model_1_exec']}  
### For the second lang. id model 
bash locally_curled.sh $log_name_2 ${config['lang_model_2_exec']}  

## After getting the logs we generate the .dat file that makes the comparaison between two langauge models.
bash generate_graph_cc_curl_lang_cmp.sh ../logs/$log_name_1 ../logs/$log_name_2 compare_${config['lang_model_1_name']}_${config['lang_model_1_name']}.dat

## we generate the image. and have a param to open it too. 
