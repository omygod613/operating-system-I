import os
import string
import random
import tempfile
import shutil
import sys

KEY_LEN = 10
string_lowercase = "abcdefghijklmnopqrstuvwxyz"
dir_name = "StartPython"

if (os.path.exists(dir_name)):
    shutil.rmtree(dir_name)
os.makedirs(dir_name)
#path = os.getcwd()
dir_path = os.path.join(os.getcwd(), dir_name)

for i in range(3):
	keylist = [random.choice(string_lowercase) for i in range(KEY_LEN)]
	keylist = "".join(keylist)
	file = open(os.path.join(dir_path, keylist), 'wb') 
	print(keylist)
	file.close()


rand_num1 = random.randint(1, 42)
rand_num2 = random.randint(1, 42)
product = rand_num1 * rand_num2
print(rand_num1)
print(rand_num2)
print(product)