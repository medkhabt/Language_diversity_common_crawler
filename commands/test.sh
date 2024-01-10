#! bin/bash 


while getopts ":h" option;do 

case $option in
      h) # display Help
	echo " yo" 
         exit;;
   esac
done

echo "Hello world!"
