#!/bin/python3

from os import listdir, remove, path

output_path = "./output"

for file_name in listdir(output_path):
	remove(path.join(output_path, file_name))
