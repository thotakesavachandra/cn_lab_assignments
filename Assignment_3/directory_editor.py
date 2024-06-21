# open a file and print line by line

import sys
import os

with open('user.txt') as f:
    for line in f:
        #get 1st word
        word = line.split()[0]
        # create a dir with the name of the word
        # remove the dir forcefully if it exists
        if os.path.exists(word):
            os.system('rm -rf ' + word)
        os.mkdir(word)
        # create a file with the name of the word
        f2 = open(word + '/mymailbox', 'w')
