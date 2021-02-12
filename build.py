#!/usr/bin/env python

import os
import subprocess
import json

compiler = "clang++"
version = "-std=c++17"
output_dir = "output"
common_dir = "common"
engine_dir = "engine"
modules_dir = "modules"

if (not os.path.exists(output_dir)):
	os.mkdir(output_dir)

if (not os.path.exists(common_dir)):
	os.mkdir(common_dir)

if (not os.path.exists(engine_dir)):
	os.mkdir(engine_dir)

if (not os.path.exists(modules_dir)):
	os.mkdir(modules_dir)

root_path = os.getcwd()
output_path = os.path.abspath(output_dir)
common_path = os.path.abspath(common_dir)
engine_path = os.path.abspath(engine_dir)
modules_path = os.path.abspath(modules_dir)
generated_file_path = os.path.join(output_path, "generated.cpp")
compile_command = [compiler, version, ("-I" + os.path.abspath(".")), "-c", "-g"]

# Compile common library.
output_file_path = os.path.join(output_path, "libcommon.a")
output_file_mod_time = (os.path.getmtime(output_file_path) if os.path.exists(output_file_path) else 0)
needs_rebuild = (os.path.getmtime(os.path.join(root_path, "common.hpp")) > output_file_mod_time)
source_file_names = []
object_file_names = []

library_file_paths = [output_file_path]

os.chdir(common_path)

for file_name in os.listdir(common_path):
	if (file_name.endswith(".cpp")):
		object_file_name = (os.path.splitext(file_name)[0] + ".o")

		source_file_names.append(file_name)
		object_file_names.append(object_file_name)

		if (not needs_rebuild):
			needs_rebuild |= (
				(not os.path.exists(object_file_name)) or
				(os.path.getmtime(file_name) > os.path.getmtime(object_file_name))
			)
	elif (file_name.endswith(".hpp")):
		needs_rebuild |= (os.path.getmtime(file_name) > output_file_mod_time)

if (needs_rebuild):
	print("Building common...")
	subprocess.call(compile_command + source_file_names)
	subprocess.call(["ar", "rcs", output_file_path] + object_file_names)

output_file_path = os.path.join(output_path, "engine")
output_file_mod_time = (os.path.getmtime(output_file_path) if os.path.exists(output_file_path) else 0)
needs_rebuild |= (os.path.getmtime(os.path.join(root_path, "engine.hpp")) > output_file_mod_time)
source_file_names = []
object_file_names = []

os.chdir(engine_path)

for file_name in os.listdir(engine_path):
	if (file_name.endswith(".cpp")):
		object_file_name = (os.path.splitext(file_name)[0] + ".o")

		source_file_names.append(file_name)
		object_file_names.append(object_file_name)

		if (not needs_rebuild):
			needs_rebuild |= (
				(not os.path.exists(object_file_name)) or
				(os.path.getmtime(file_name) > os.path.getmtime(object_file_name))
			)
	elif (file_name.endswith(".hpp")):
		needs_rebuild |= (os.path.getmtime(file_name) > output_file_mod_time)

if (needs_rebuild):
	print("Building engine...")
	subprocess.call(compile_command + source_file_names)

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
