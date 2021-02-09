#!/usr/bin/env python

import os

for directory in ["common", "engine"]:
	for file_name in os.listdir(directory):
		if (file_name.endswith(".o")):
			os.remove(os.path.join(directory, file_name))
