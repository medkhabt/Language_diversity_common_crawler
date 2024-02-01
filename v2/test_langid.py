import langid
import sys

import pycld2 as cld2  
if len(sys.argv) > 1: 
    print(langid.classify(sys.argv[1]))
#    print(cld2.detect(sys.argv[1])[2][0])
print(help(langid.classify))
#dic = [] 
#print(type(dic))
#print(help(cld2.detect))
