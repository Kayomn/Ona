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

output_modules_dir = os.path.join(output_dir, modules_dir)

if (not os.path.exists(output_modules_dir)):
	os.mkdir(output_modules_dir)

root_path = os.getcwd()
output_path = os.path.abspath(output_dir)
compile_command = [compiler, version, ("-I" + os.path.abspath(".")), "-c", "-g"]
needs_rebuild = False

class Linker:
	def __init__(self, namer, action) -> None:
		self.namer = namer
		self.action = action

lib_linker = Linker(
	action = lambda object_file_names, flags, output_file_path : subprocess.call([
		"ar",
		"rcs",
		output_file_path
	] + object_file_names),

	namer = lambda output_file_name : output_file_name
)

dll_linker = Linker(
	action = lambda object_file_names, flags, output_file_path : subprocess.call([
		compiler,
		version,
		"-g",
		"-shared",
		"-o" + output_file_path,
	] + object_file_names + flags),

	namer = lambda output_file_name : output_file_name + ".so"
)

exe_linker = Linker(
	action = lambda object_file_names, flags, output_file_path : subprocess.call([
		compiler,
		version,
		"-g",
		"-o" + output_file_path
	] + object_file_names + flags),

	namer = lambda output_file_name : output_file_name
)

def build_component(
	component_name: str,
	flags: list,
	output_file_path: str,
	linker: Linker
) -> bool:
	global needs_rebuild
	component_path = os.path.join(root_path, component_name)

	if (not os.path.exists(component_path)):
		os.mkdir(component_path)

	header_extension = ".hpp"
	output_file_path = linker.namer(output_file_path)
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
		linker.action(object_file_names, flags, output_file_path)

	os.chdir(root_path)

common_lib_file_path = os.path.join(output_path, "onacommon.a")
output_modules_path = os.path.abspath(output_modules_dir)

library_file_paths = [
	"-lSDL2",
	"-ldl",
	"-lGL",
	"-lGLEW",
	"-lpthread",
	common_lib_file_path
]

build_component("common", [], common_lib_file_path, lib_linker)

for file_name in os.listdir(modules_dir):
	module_path = os.path.join(modules_dir, file_name)

	if (os.path.isdir(module_path)):
		build_component(module_path, [], os.path.join(output_modules_path, file_name), dll_linker)

build_component("engine", library_file_paths, os.path.join(output_path, "engine"), exe_linker)
