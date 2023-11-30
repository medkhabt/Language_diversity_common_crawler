#!/bin/bash

# get the file 


# get only the url from the file. 

echo "start of the script curl"
output_file="result_curl"
rm $output_file.log 2>/dev/null 
sed 's/\(.*\)::.*::.*::.*/\1\n/g' $1 > seed.tmp 

# get the html page 

while IFS= read -r line 
do 
    echo "start curling"
    if [ "$line" != "" ]
    then
    echo "$line::::" > cleanest_page.html.tmp
	python3 boilerplate_removal.py "$line" >> cleanest_page.html.tmp
    echo "after the boilerplate removal"
    sh lang_id.sh  cleanest_page.html.tmp $output_file 
    fi
    echo "end execution of language identification"
done < seed.tmp 


echo "end of the script curl"
#content = "{}::{}::{}\n{} ".format(
#		record.rec_headers.get_header('WARC-Target-URI'),
#		"" if ( lang == None) else lang.split(',', 1)[0], 
#		record.rec_headers.get_header('Content-Length'),	
#		a.decode('utf-8').replace("'", "`")
#            )


