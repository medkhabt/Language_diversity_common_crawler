#! /bin/bash 

warc=$(sed 's/|/\n/g;q' $1 | nl | grep "meta" | head -n 1 | cut -f1 | tr -d ' ')
tail -n +2 $1 | awk -F '|'  -v x=$(echo $warc) '$x!= "-" {print $0}' > $1_with_meta 
tail -n +2 $1 | awk -F '|'  -v x=$(echo $warc) '$x== "-" {print $0}' > $1_without_meta 
