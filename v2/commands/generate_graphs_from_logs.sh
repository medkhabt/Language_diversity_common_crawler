#! /bin/bash 
# I  could of used awk but wanted to change a bit.
output_name=$(echo "$1" | rev | cut -d "/" -f1 | rev | cut -d "." -f1)
bash count_compare_result.sh $1 ../graphs/dat/$output_name
bash fasrawc_generetate_graph_comparaison.sh $output_name  compare_warc_curl

