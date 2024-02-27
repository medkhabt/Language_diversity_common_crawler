#! /bin/bash 

## Args 1 : dat file name 
dat_file_with_path=graphs/dat/$1.dat
## Args 2 : dem file name 
dem_file_with_path=graphs/dem/$2.dem

## access the dem folder .
## cd ../graphs/dem 2> /dev/null
## Gen dir and  Generate the file splits for the specified file in args. 
rm -r graphs/dat/.$1 2>/dev/null
mkdir graphs/dat/.$1 2> /dev/null
## Run the split script 
lines=$(wc -l $dat_file_with_path | awk '{print $1}')
file_name=$(echo $1 | awk -F . '{print $1}' )
let "splits = (lines/10) + 1"
for ((i=0 ; i < $splits; i++)); do 
    let "x=i*10+2"
    head -n 1 $dat_file_with_path > graphs/dat/.$1/${1}_$i.dat
    tail -n +$x $dat_file_with_path | head -n 10 >> graphs/dat/.$1/${1}_$i.dat
done

## Get all the files in a list 
# TODO check if this works.
split_files=$(ls graphs/dat/.$1 -pl | grep -v / | awk '{if(NR-1>0) print $NF}')

cd graphs/dem

for file in $split_files 
do 
     gnuplot -e "filename='../dat/.$1/$file'" $2.dem
done 
## loop on all of them using the specified dem file. 

## mv the png files to the root graph dir . 
rm -r ../images/$1  2> /dev/null
mkdir ../images/$1 2> /dev/null
mv ../dat/.$1/*png ../images/$1
## rm the cached generated folder for the dat file .
rm -r ../dat/.$1
## get back to the command folder where we started .
cd ../../commands

