
## calculate the ratio of language change in both cases. 
## get the languages from the default config file and check each one of them . For the firt implementation, it is better to just go with one language identification model.
## TODO make language identification variable.
## search for the col numbers of a language identification model result.
warc=$(sed 's/|/\n/g;q' $1 | nl | grep "detect_fast" | head -n 1 | cut -f1 | tr -d ' ')
curl=$(sed 's/|/\n/g;q' $1 | nl | grep "detect_fast" | tail -n 1 | cut -f1 | tr -d ' ')

echo "warc lang col is $warc and curl lang col is $curl"

# THOSE are stats. should not be in this file.


ratio_change=$(awk -F '|' -v x=$(echo $warc) -v y=$(echo $curl) '$x != $y {countDiff++}END{print("the ratio of change is :" countDiff*100/NR "%")}' $1) 

echo "$ratio_change"
length_target=$(wc -l $1 | cut -d ' ' -f1)
echo "$length_target"
length_whole_file=$(wc -l $2  | cut -d ' ' -f1)
echo "$length_whole_file"
ratio_redirect=$((length_target * 100 / length_whole_file))
echo "$ratio_redirect"
echo " the ratio of urls that fit the target  is $ratio_redirect %"
