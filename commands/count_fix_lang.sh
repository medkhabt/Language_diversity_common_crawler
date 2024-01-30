#! /bin/bash
## First arg is the input file
## Second arg is the output file name without extension.
## Third arg is the number representing the model to fix 
## [detect_fast : 0 , langid : 1, cld2: 2] 
## Fourth is the language to fix. 
## First line should be the column with the names.
head -n 1 $1 | awk -F '|' '{print "lang",$3,$4,$5}' > $2.dat
let "index = $3 + 3"
#head  -n 1 $1| awk -v idx="$index" -F '|' '{print idx}' 
tail $1 -n +2 | awk -v idx="$index" -v lang="$4" -F '|' '{if($idx == lang){res1[$((idx-3+1)%3 + 3)]++; res2[$((idx-3+2)%3 + 3)]++}} END {for(key in res1){print  key, ((idx-3+1)%3), res1[key]};for(key in res2){print key,((idx-3+2)%3), res2[key]}p}' | sort -k3 -rn 
#tail $1 -n +2 | awk -v idx="$index" -v lang="$4" -F '|' '{if($index == lang){res1[$((index-3+1)%3 + 3)]++; res2[$((index-3+2)%3 + 3)]++}}' 

#tail $1 -n +2 |awk -F '|' '{first[$3]++;second[$4]++;third[$5]++;if(lang[$3] == 0) {lang[$3] = $3} if(lang[$4] == 0){lang[$4]=$4} if(lang[$5]==0){lang[$5]=$5}  }'END'{for (i in lang) {print i, (i in first) ? first[i] : 0, (i in second ) ? second[i]: 0, (i in third) ? third[i]: 0} }'  >> $2.dat
