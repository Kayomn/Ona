#!/usr/bin/env python

import os
import subprocess
import json

compiler = "clang++"
version = "-std=c++17"
output_dir = "output"
modules_dir = "modules"

if (not os.path.exists(output_dir)):
	os.mkdir(output_dir)

if (not os.path.exists(modules_dir)):
	os.mkdir(modules_dir)

root_path = os.getcwd()
output_path = os.path.abspath(output_dir)
compile_command = [compiler, version, ("-I" + os.path.abspath(".")), "-c", "-g"]
needs_rebuild = False

def link_lib(object_file_names, output_file_path):
	subprocess.call(["ar", "rcs", output_file_path] + object_file_names)

def link_exe(object_file_names, output_file_path):
	subprocess.call([
		compiler,
		version,
		"-g",
		("-o" + output_file_path),
		"-lSDL2",
		"-ldl",
		"-lGL",
		"-lGLEW",
		"-lpthread"
	] + object_file_names + library_file_paths)

def build_component(component_name: str, output_file_path: str, linker) -> bool:
	global needs_rebuild
	component_path = os.path.join(root_path, component_name)

	if (not os.path.exists(component_path)):
		os.mkdir(component_path)

	header_extension = ".hpp"
	output_file_mod_time = (os.path.getmtime(output_file_path) if os.path.exists(output_file_path) else 0)
	needs_rebuild |= (os.path.getmtime(component_path + header_extension) > output_file_mod_time)
	source_file_names = []
	object_file_names = []

	os.chdir(component_path)

	for file_name in os.listdir(component_path):
		if (file_name.endswith(".cpp")):
			object_file_name = (os.path.splitext(file_name)[0] + ".o")

			source_file_names.append(file_name)
			object_file_names.append(object_file_name)

			if ((not needs_rebuild) and (
				(not os.path.exists(object_file_name)) or
				(os.path.getmtime(file_name) > os.path.getmtime(object_file_name))
			)):
				needs_rebuild = True
		elif (file_name.endswith(header_extension)):
			if ((not needs_rebuild) and (os.path.getmtime(file_name) > output_file_mod_time)):
				needs_rebuild = True

	if (needs_rebuild):
		print("Building %s..." % component_name)
		subprocess.call(compile_command + source_file_names)
		linker(object_file_names, output_file_path)

	os.chdir(root_path)

# Compile common library.
output_file_path = os.path.join(output_path, "libcommon.a")
library_file_paths = [output_file_path]

build_component("common", output_file_path, link_lib)
build_component("engine", os.path.join(output_path, "engine"), link_exe)
