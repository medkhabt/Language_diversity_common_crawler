# generate the counts of each log file 
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd );

sed '/^[[:space:]]*$/d' $1 > $1.tmp
bash pre_step_for_count.sh $1.tmp > .first_count.tmp 
rm $1.tmp

echo "the first count is" 
cat .first_count.tmp

sed '/^[[:space:]]*$/d' $2 > $2.tmp 
bash pre_step_for_count.sh $2.tmp > .second_count.tmp 
rm $2.tmp

echo "the second count is" 
cat .second_count.tmp

bash concat_count.sh .first_count.tmp .second_count.tmp > $SCRIPT_DIR/../graphs/$3.dat 

# clean up
rm .first_count.tmp .second_count.tmp




