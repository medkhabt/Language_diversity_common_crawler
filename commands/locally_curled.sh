#!/bin/bash

# get the file 


# get only the url from the file. 

echo "start of the script curl"

# first arg is result_log to process
# second arg is the script used for the language identification
# third arg is the name of the output file
output_file=$3
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );
LOGS_DIR="$SCRIPT_DIR/../logs"
rm $LOGS_DIR/$output_file.log 2>/dev/null 

cat $LOGS_DIR/$1 > $LOGS_DIR/local_$1

sed 's/\(.*\)::.*::.*::.*/\1\n/g' $LOGS_DIR/$1 | sed '/^[[:space:]]*$/d' > .seed.tmp 

# get the html page 

while IFS= read -r line 
do 
    
    if [ "$line" != "" ]
    then
    STATS="$line::::"
	python3 boilerplate_removal.py "$line" > .content.tmp 
    
    # I need to check here if we had an exception or if after the removal the text has nothing inside 
	content=$( head -n 1 .content.tmp  )
	
	if [[ "$content" == "" ]]
	then
	    content="EMPTY";
	fi
	if [ "$content" == "FAILED" ] || [ "$content" == "EMPTY" ] 
	then 
	    if [ "$content" == "FAILED" ]
	    then 
		# Remove the url from the common_crawl result as it is no longer available, to match the stats
		# TODO it is slow to remove each line at a time, it would be better to save the lines to delete and delete them at once.
		# TODO we should save the failed urls in an other file, as it does not belong to a file related to a language identification model
		sed "/$(echo $line | sed "s,\/,\\\/,g" )/d" $LOGS_DIR/$1 > .$1.tmp
		
		cat .$1.tmp> $LOGS_DIR/$1
	    fi
		# TODO I need to change the name of this log, as it needs to be variable to the boilerplate removal technique.
		sed "/$(echo $line | sed "s,\/,\\\/,g" )/d" $LOGS_DIR/local_$1 > .$1.tmp
		cat .$1.tmp > $LOGS_DIR/local_$1
	    echo "$STATS::$content" >> $LOGS_DIR/$output_file.log 
		
	else 
	    echo "$STATS"> .cleanest_page.html.tmp
	    echo "$content" >> .cleanest_page.html.tmp 
	    # the language identification script
	    sh $2 .cleanest_page.html.tmp $output_file 
	    
	fi 
    fi
done < .seed.tmp 


echo "end of the script curl"
#content = "{}::{}::{}\n{} ".format(
#		record.rec_headers.get_header('WARC-Target-URI'),
#		"" if ( lang == None) else lang.split(',', 1)[0], 
#		record.rec_headers.get_header('Content-Length'),	
#		a.decode('utf-8').replace("'", "`")
#            )


