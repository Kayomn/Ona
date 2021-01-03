#!/bin/python3

from subprocess import call
from os import path, listdir, mkdir

common_flags = ["-g", "-fno-exceptions", "-std=c++17", "-I.", "-fPIC"]
extensions_path = "ext"
compiler = "clang++"

if (not path.exists(extensions_path)):
	mkdir(extensions_path)

for file_name in listdir(extensions_path):
	extension_name = path.splitext(file_name)[0]
	source_path = path.join("ext", file_name)
	library_path = path.join("assets", (extension_name + ".so"))

	if (
		not path.exists(library_path) or
		(path.getmtime(source_path) > path.getmtime(library_path))
	):
		print("Recompiling extension:", extension_name)

		call([compiler, source_path, "-shared", ("-o" + library_path)] + common_flags)
