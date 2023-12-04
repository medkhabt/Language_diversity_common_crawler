#! /bin/bash 

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );

LOG_DIR="$SCRIPT_DIR/../logs"


STATS=$( head -n 1 $1 )
tail -n +2 "$1" > ".$1.tmp"

lang=$($SCRIPT_DIR/externals/whatlang_id ".$1.tmp")
echo "${lang:0:3}" > .predic.tmp

log_result=$(echo "${STATS}::$( head -n 1 .predic.tmp)")
echo "$log_result" >> "$LOG_DIR/$2.log"

# Clean up 
rm ".$1.tmp" .predic.tmp 


