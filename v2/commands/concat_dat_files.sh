
output_name="$(echo "$1" | rev | cut -d "/" -f1 | rev | cut -d "." -f1)$(echo "$2" | rev | cut -d "/" -f1 | rev | cut -d "." -f1)"

echo "${output_name}"

head -n 1 $1 | awk -F '|' '{print "lang","cc",$3,"first","second"}' > concat_${output_name}.dat
paste $1 $2 | tail -n +2 | awk '{lang[$1] = $1; cc[$1] = $2; i1[$1] = $3; lang[$4] = $4; i2[$4]=$6}'END'{for (i in lang) {print i, (i in cc) ? cc[i] : 0, (i in i1) ? i1[i]: 0, (i in i2) ? i2[i]: 0 }}'  >> concat_${output_name}.dat
