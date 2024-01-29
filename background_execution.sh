#! /bin/bash 
nohup python3 sequential_cc_fastwarc.py $1 $2 &> nohup_$2.out &
