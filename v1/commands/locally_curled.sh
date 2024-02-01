#!/bin/bash

# get the file 


# get only the url from the file. 
# INPUTS 
# 1 : input file name  
# 2 : the language identifiation binary 
# 3 : the output file name 
# *******
echo "start of the script curl"

# first arg is result_log to process
# second arg is the script used for the language identification
# third arg is the name of the output file
TEMP_DIR=$(hostname) 
mkdir $TEMP_DIR
output_file=$3
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );
LOGS_DIR="$SCRIPT_DIR/../logs"
rm $LOGS_DIR/$output_file.log 2>/dev/null 

cat $LOGS_DIR/$1.log > $LOGS_DIR/local_$1.log

sed 's/\(.*\)::.*::.*::.*/\1\n/g' $LOGS_DIR/$1.log | sed '/^[[:space:]]*$/d' > $TEMP_DIR/.seed.tmp 

# get the html page 
process_index=0
line_index=0
chunk=0
file_length=$(wc -l $LOGS_DIR/$1.log | awk '{print $1}')
file_length_minus=$((file_length - 1))
let "num_chunks = ${file_length} / 139"
echo "Expected number of chunks is $num_chunks"
rm .all_contnents.tmp 2> /dev/null
while IFS= read -r line 
do  
    if [ "$line" != "" ]
    then
	(python3 boilerplate_removal.py "$line" > $TEMP_DIR/.content_${line_index}.tmp) &
	pids[${process_index}]=$!
	let "line_index = line_index + 1"
	let "process_index=process_index+1"
	# Do a chunk of 140 in parallel, from experiments, that's the faster we can do 
	if [[ $process_index -eq 139 || ($file_length -le 139 && $process_index -eq $file_length_minus) ]]; then 
	    echo "waiting for chuck $chunk to finish"
	    for pid in ${pids[*]}; do 
		wait $pid
	    done
	    # concat all the chunk results and recycle them for the next wave.
	    echo "chunk $chunk is done"
	    let "process_index = 0" 
	    let "chunk = chunk + 1"
	fi
    fi
done < $TEMP_DIR/.seed.tmp 


# i don't think we need to do this part in parallel.
# TODO watch out, in case we change the order of the file in between!
line_index=0
ulimit -n 4096
while IFS= read -r line 
do  # *********************** START WHILE ***************
    STATS="$line::::"
    
    # I need to check here if we had an exception or if after the removal the text has nothing inside 
	content=$( head -n 1 $TEMP_DIR/.content_${line_index}.tmp  )
	let "line_index = line_index+1"
	
	if [[ "$content" == "" ]]
	then
	    content="EMPTY";
	fi
	# ************ START FIRT LEVEL CONDITION **********
	if [ "$content" == "FAILED" ] || [ "$content" == "EMPTY" ] 
	then 
	    # *********** START SECOND LEVEL CONDITION ********
	    if [ "$content" == "FAILED" ]
	    then 
		# Remove the url from the common_crawl result as it is no longer available, to match the stats
		# TODO it is slow to remove each line at a time, it would be better to save the lines to delete and delete them at once.
		# TODO we should save the failed urls in an other file, as it does not belong to a file related to a language identification model
		sed "/$(echo $line | sed "s,\/,\\\/,g" )/d" $LOGS_DIR/$1.log > .$1.tmp
		
		cat .$1.tmp> $LOGS_DIR/$1.log
	    fi # ********** FINISH SECOND LEVEL CONDITION
		# TODO I need to change the name of this log, as it needs to be variable to the boilerplate removal technique.
		sed "/$(echo $line | sed "s,\/,\\\/,g" )/d" $LOGS_DIR/local_$1.log > .$1.tmp
		cat .$1.tmp > $LOGS_DIR/local_$1.log
	    echo "$STATS::$content" >> $LOGS_DIR/$output_file.log 
		
	else # ********* ELSE FIRST LEVEL CONDITION 
	    echo "$STATS"> $TEMP_DIR/.cleanest_page.html.tmp
	    echo "$content" >> $TEMP_DIR/.cleanest_page.html.tmp 
	    # the language identification script

# TODO i will have a problem in the langclass id exe. wit the arg 2
	    bash lang_id_$2.sh  $TEMP_DIR/.cleanest_page.html.tmp $output_file 2> $TEMP_DIR/error.log &
	    wait $!
	fi # ************** FIRST LEVEL CONDITION 
done < $TEMP_DIR/.seed.tmp 

echo "end of the script curl"
#content = "{}::{}::{}\n{} ".format(
#		record.rec_headers.get_header('WARC-Target-URI'),
#		"" if ( lang == None) else lang.split(',', 1)[0], 
#		record.rec_headers.get_header('Content-Length'),	
#		a.decode('utf-8').replace("'", "`")
#            )

rm -rf $TEMP_DIR
