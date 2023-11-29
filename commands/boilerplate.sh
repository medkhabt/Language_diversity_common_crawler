#! /bin/bash 


# curl and convert to the right encoding

charset=curl -I  http://0-50.ru/news/incident/2013-04-06/id_31240.html | grep "charset" | sed 's/.*charset=\(.*\)/\1/g'
if [ charset!= "utf-8" ]
then 
    curl $1 |  iconv -f windows-1251 -t utf-8
else 
    curl $1 



