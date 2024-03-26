#! /bin/bash

## get the specific log 
log_file=$1
## serach for the col (redirect) 
col=$(sed 's/|/\n/g;q' $log_file | nl | grep "redirect" | cut -f1)
echo "the col number is ${col}"
## filter out just the rows with a redirections and do the reverse of that.
#awk -v col_n=col_number '{if($col_n == 'yes'){print $0}}' $log_file > result.log
head -n 1 $1 > ${log_file}_redirect_true.log
tail -n +2 $log_file | awk -F '|' -v x=$(echo $col) '$x == "yes" {print $0}'  >> ${log_file}_redirect_true.log
head -n 1 $1 > ${log_file}_redirect_false.log
tail -n +2 $log_file | awk -F '|' -v x=$(echo $col) '$x == "no" {print $0}' $log_file >> ${log_file}_redirect_false.log


