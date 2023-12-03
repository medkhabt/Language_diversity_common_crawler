# generate the counts of each log file 
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );

bash pre_step_for_count.sh $1 > .first_count.tmp 
bash pre_step_for_count.sh $2 > .second_count.tmp 

bash concat_count.sh .first_count.tmp .second_count.tmp > $SCRIPT_DIR/../graphs/concat_cc_curl.dat 

# clean up
rm .first_count.tmp .second_count.tmp




