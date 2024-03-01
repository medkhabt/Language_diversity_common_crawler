#! /bin/bash 

head -n 1 $1 | awk -F '|' '{print "lang",$3,$6}' > $2.dat
tail $1 -n +2 |awk -F '|' '{warc_count[$3]++;curl_count[$6]++; if(lang[$3] == 0) {lang[$3] = $3} if(lang[$6] == 0){lang[$6]=$6} }'END'{for (i in lang) {print i, (i in warc_count) ? warc_count[i] : 0, (i in curl_count) ? curl_count[i]: 0  }}'  >> $2.dat
