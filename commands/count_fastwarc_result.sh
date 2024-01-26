#! /bin/bash
## First arg is the input file
## Second arg is the output file name without extension.
awk '{first[$3]++;second[$4]++;third[$5]++;if(lang[$3] == 0) {lang[$3] = $3} if(lang[$4] == 0){lang[$4]=$4} if(lang[$5]==0){lang[$5]=$5}  }'END'{for (i in lang) {print i, (i in first) ? first[i] : 0, (i in second ) ? second[i]: 0, (i in third) ? third[i]: 0} }' $1  > $2.dat
