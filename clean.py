#!/bin/python3

from os import listdir, remove, path

output_path = "output"

if (path.exists(output_path)):
	for filename in listdir(output_path):
		remove(path.join(output_path, filename))
