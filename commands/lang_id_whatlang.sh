#! /bin/bash 

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" 2> /dev/null && pwd );
# if [["$DIR_NAME" = "commands"]]; then 
#    LOG_DIR="$SCRIPT_DIR/logs"
#else 
#    LOG_DIR="$SCRIPT_DIR/../logs"
#fi

    LOG_DIR="$SCRIPT_DIR/../logs"
# TODO is this just for the cc dump , i don't remember, but i should not execute this part if the file is generated from curl.
STATS=$( head -n 1 $1 )
tail -n +2 "$1" > ".$1.tmp"
#TODO I replaced .$1.tmp by $1 in the next instruction to make it work for my case,  idk why i did the otehr one, i might have broken soemthing.
lang=$($SCRIPT_DIR/externals/whatlang_id "$1")
#echo "${lang:0:3}" > .predic.tmp
echo "${lang}" > .predic.tmp

log_result=$(echo "${STATS}::$( head -n 1 .predic.tmp)")
echo "$log_result" >> "$LOG_DIR/$2.log"

# Clean up 
rm .predic.tmp 2> /dev/null 


