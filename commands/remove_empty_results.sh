#! /bin/bash
# while if failed sed the link in the other file 
curl_log=$2
cc_log=$1
cat $1 > .new_cc.log
echo "curl log is $curl_log" 
while IFS= read -r line 
do
    STATE=$(echo $line | awk -F '::::::' '{print $2}') 
    LINK=$(echo $line | awk -F '::::::' '{print $1}') 
    if [ "$STATE"="EMPTY" ]; then
	echo "the whole line is $line"
	echo "replacing $LINK"
	echo ".new_cc.log before $(wc -l .new_cc.log)" 
	sed "\@$LINK@d" .new_cc.log > .tmp 
	cat .tmp > .new_cc.log
	echo ".new_cc.log after $(wc -l .new_cc.log)" 
    fi

done < $curl_log
