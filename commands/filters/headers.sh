#! /bin/bash 
# You can also specify a header, so you fill filter just the occurences of that header

warc=$(sed 's/|/\n/g;q' $1 | nl | grep "header" | head -n 1 | cut -f1 | tr -d ' ')
echo "number of args: $#" 
# TODO need to make this part work ( with a specific header) 
if [ "$#" -eq 2 ]; then 
   tail -n +2 $1 | awk -F '|'  -v x=$(echo $warc) y=$(echo $2)'$x== y {print $0}' > $1_with_headers 
else 
   tail -n +2 $1 | awk -F '|'  -v x=$(echo $warc) '$x!= "-" {print $0}' > $1_with_headers 
   tail -n +2 $1 | awk -F '|'  -v x=$(echo $warc) '$x== "-" {print $0}' > $1_without_headers 
fi
