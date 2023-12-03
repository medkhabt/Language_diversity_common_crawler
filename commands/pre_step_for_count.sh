cat $1 |  sed "s/.*::\([^:]*\)$/\1/g" > .pre_count.tmp

bash count_uniq_lines.sh .pre_count.tmp | sort 
