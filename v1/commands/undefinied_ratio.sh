#! /bin/bash

## we can run this on the dat files generated from the logs based on the curled urls. 
## the input must be carefully choosen , no pre check is included in the script!
output=$(grep 'EMPTY .*' $1 | sed 's/EMPTY[[:space:]]*\([0-9]*\)/\1/g') 
echo "$output"
