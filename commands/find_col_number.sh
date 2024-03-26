sed 's/|/\n/g;q' $1 | nl | grep $2 | cut -f1
