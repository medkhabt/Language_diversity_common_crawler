# generate the counts of each log file 
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );


## For the first file
sed '/^[[:space:]]*$/d' $1 > $1.tmp
bash pre_step_for_count.sh $1.tmp > .first_count.tmp 

#echo "the first count is $(cat .first_count.tmp)"

while IFS= read -r line 
do 
    lang=$( echo $line | awk -F " " '{print $1}')  
    key_lang=$(cat /Users/medkhalil/dev/phd_track/wet_parser_python/data/language_identification/languages_mapping.txt | grep -w "$lang" | awk -F " " '{print $1}');
    echo "the key language is '$key_lang'" 
     if [[ "$key_lang" != "" ]]
	    then
	    echo "the line is $line" 
		new_line=$(echo $line | sed "s/$lang/$key_lang/g")
	    if [[ "$new_line" != "" ]] 
		then 
		echo "the new line is $new_line"
		echo $new_line >> .converted_first_count.tmp
	    fi    
    fi
done < .first_count.tmp
rm $1.tmp


######################## SECOND FILE 

## For the second file
sed '/^[[:space:]]*$/d' $2 > $2.tmp
bash pre_step_for_count.sh $2.tmp > .second_count.tmp 


echo "################### the start of the second file"

while IFS= read -r line 
do 
    lang=$( echo $line | awk -F " " '{print $1}')  
    echo "the Language is : $lang"
    key_lang=$(cat /Users/medkhalil/dev/phd_track/wet_parser_python/data/language_identification/languages_mapping.txt | grep -w "$lang" | awk -F " " '{print $1}');
    echo "the key language is '$key_lang'" 
     if [[ "$key_lang" != "" ]]
	    then
	    echo "the line is $line" 
		new_line=$(echo $line | sed "s/$lang/$key_lang/g")
	    if [[ "$new_line" != "" ]] 
		then 
		echo "the new line is $new_line"
		echo $new_line >> .converted_second_count.tmp
	    fi    
    fi
done < .second_count.tmp
rm $2.tmp

################## The concatination between the two files.
echo "done with second file"
bash concat_count.sh .converted_first_count.tmp .converted_second_count.tmp > $SCRIPT_DIR/../graphs/$3.dat 
# clean up
rm .converted_first_count.tmp .converted_second_count.tmp




