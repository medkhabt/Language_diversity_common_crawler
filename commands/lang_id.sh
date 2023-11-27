#! /bin/bash 


SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );
# Remove the first line that will containe the info about the bundle 

STATS=$( head -n 1 $1 )
tail -n +2 "$1" > "$1.tmp"

# I think it is much slower to clean up the output instead of at the end when we get our result file. 

($SCRIPT_DIR/testtextcat $SCRIPT_DIR/fpdb.conf < "$1.tmp") | sed 's/\[\([^-]*\)-*[^-]*-utf8\].*/\1/g' > predic.tmp


echo "${STATS} :: $( head -n 1 predic.tmp) " >> $SCRIPT_DIR/result.log
# rm "$1.tmp"
# Add that line in the result log for stats. 
