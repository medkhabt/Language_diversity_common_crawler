#!/bin/bash 

# we need a seed first 
# input="$1.tmp"; 
input=$1

declare -A count_words;
while IFS= read -r line 
do
	if [[ $line != "" ]]
	    then 
	    if [[ -z "${count_words[$line]}" ]]
		then 
			let "count_words[$line]=1"
		else
			let "count_words[$line]=${count_words[$line]} + 1"
		fi
	fi
			
done < "$input"

for index in "${!count_words[@]}"; do echo "$index    ${count_words[$index]}"; done

