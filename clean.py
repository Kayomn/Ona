#!/bin/python3

from os import listdir, remove, path

assets_path = "assets"
output_path = "output"

# Generated asset files.
if (path.exists(assets_path)):
	for filename in listdir(assets_path):
		if (filename.endswith(".so") or filename.endswith(".dll")):
			remove(path.join(assets_path, filename))

# Engine binaries.
if (path.exists(output_path)):
	for filename in listdir(output_path):
		remove(path.join(output_path, filename))
