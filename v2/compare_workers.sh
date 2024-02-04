#! /bin/bash 

for ((i= 0; i<=300; i=i+10)) do 	
    echo "*********** $i workers ***************"
    time python3 concurrent_compare_curl_warc.py -s 00009 -w $i -n 1000 
    echo "\n\n"
done 
