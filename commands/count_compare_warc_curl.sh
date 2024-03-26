#! /bin/bash 
log_file=$1
#TODO do it for multiple languages
# Specify the columns that we need to look for first
. commands/parser_ini.sh
lang=$(iniget default.ini LANGUAGES | grep "^[^#]" | cut -d '=' -f 1)
# found the language name, now we need to find the number of the col with the number name.
echo "lang is $lang"
# I need to get all the language identification models that i am working with. and filter on each one of them 
# For now let's assume first that we only use one lang ide model. 
warc=$(sed 's/|/\n/g;q' $1 | nl | grep $lang | head -n 1 | cut -f1 | tr -d ' ') 
curl=$(sed 's/|/\n/g;q' $1 | nl | grep $lang | tail -n 1 | cut -f1 | tr -d ' ') 

head -n 1 $1 | awk -F '|' -v x=$(echo $warc) -v y=$(echo $curl) '{print "lang",$x,$y}' > $2.dat
tail $1 -n +2 |awk -F '|' -v x=$(echo $warc) -v y=$(echo $curl) '{warc_count[$x]++;curl_count[$y]++; if(lang[$x] == 0) {lang[$x] = $x} if(lang[$y] == 0){lang[$y]=$y} }'END'{for (i in lang) {print i, (i in warc_count) ? warc_count[i] : 0, (i in curl_count) ? curl_count[i]: 0  }}'  >> $2.dat
