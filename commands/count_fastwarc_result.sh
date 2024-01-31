#! /bin/bash
## First arg is the input file
## Second arg is the output file name without extension.
## First line should be the column with the names.
head -n 1 $1 | awk -F '|' '{print "lang",$3,$4,$5,$6,$7,$8}' > $2.dat
tail $1 -n +2 |awk -F '|' '{first[$3]++;first_precision[$3]=first_precision[$3] + $6 ; second_precision[$4]=second_precision[$4] + $7 ;second[$4]++;third[$5]++; third_precision[$5]=third_precision[$5] + $8 ;if(lang[$3] == 0) {lang[$3] = $3} if(lang[$4] == 0){lang[$4]=$4} if(lang[$5]==0){lang[$5]=$5}  }'END'{for (i in lang) {print i, (i in first) ? first[i] : 0, (i in second ) ? second[i]: 0, (i in third) ? third[i]: 0, (i in first)? first_precision[i]/first[i]: 0, (i in second)? second_precision[i]/second[i]: 0, (i in third)? third_precision[i]/third[i]: 0 }}'  >> $2.dat
#}
