#!/bin/python3

from argparse import ArgumentParser
from os import path, listdir
from subprocess import Popen, call
import json

common_flags = ["-g", "-fno-exceptions", "-std=c++17", "-I./source"]
output_path = "output"
input_path = "source"

def name_static_lib(name: str) -> str:
	return (name + ".a")

def name_shared_lib(name: str) -> str:
	return (name + ".so")

def name_executable(name: str) -> str:
	return name

target_type_namers = {
	"static-lib": name_static_lib,
	"shared-lib": name_shared_lib,
	"executable": name_executable
}

def to_object_path(source_path: str) -> str:
	path_nodes = []

	while (source_path):
		split = path.split(source_path)
		source_path = split[0]

		path_nodes.append(split[1])

	if (len(path_nodes)):
		path_nodes[0] = (path.splitext(path_nodes[0])[0] + ".o")

		path_nodes.reverse()

		if (path_nodes[0] == "source"):
			path_nodes.pop(0)

		return path.join(output_path, ".".join(path_nodes))

	return ""

def link_shared_lib(module_name: str, module_config: dict, object_paths: list) -> None:
	print("Linking", module_name, "shared library...")

def link_static_lib(module_name: str, module_config: dict, object_paths: list) -> None:
	print("Linking", module_name, "static library...")
	call(["llvm-ar", "rc", path.join(output_path, (module_name + ".a"))] + object_paths)

def link_executable(module_name: str, module_config: dict, object_paths: list) -> None:
	print("Linking", module_name, "executable...")

	args = (["clang++"] + object_paths)

	if ("dependencies" in module_config):
		for dependency, target_type in module_config["dependencies"].items():
			if (target_type in target_type_namers):
				dependency_path = target_type_namers[target_type](
					path.join(output_path, dependency)
				)

				if (path.exists(dependency_path)):
					args.append(dependency_path)

	args += (["-o" + path.join(output_path, module_name)] + common_flags)

	if ("libraries" in module_config):
		for library in module_config["libraries"]:
			args.append("-l" + library)

	call(args)

target_type_linkers = {
	"static-lib": link_static_lib,
	"shared-lib": link_shared_lib,
	"executable": link_executable
}

target_types = [
	"static-lib",
	"shared-lib",
	"executable"
]

def build(name: str) -> bool:
	build_path = path.join(input_path, name)

	with open(path.join(build_path, "build.json")) as build_file:
		build_config = json.load(build_file)

		if (not "modules" in build_config):
			print("No modules specified in build.json for", name)
			exit(1)

		if (not "targetType" in build_config):
			print("No target type specified in build.json for", name)
			exit(1)

		target_type = build_config["targetType"]

		if (not target_type in target_types):
			print("Invalid target type specified in build.json for", name)
			exit(1)

		compilation_process_ids = []
		object_paths = []
		needs_recompile = False

		def compile_source(source_path: str, object_path: str) -> None:
			print(source_path, "->", object_path)
			object_paths.append(object_path)

			compilation_process_ids.append(Popen([
				"clang++",
				source_path,
				("-o" + object_path),
				"-c"
			] + common_flags))

		if ("dependencies" in build_config):
			for dependency in build_config["dependencies"]:
				needs_recompile |= build(dependency)

		print("Building", (name + "..."))

		binary_path = target_type_namers[target_type](path.join(output_path, name))

		if (needs_recompile):
			# A dependency has changed so recompile the entire package.
			for module in build_config["modules"]:
				folder_path = path.join(build_path, module)
				source_path = (folder_path + ".cpp")

				if (path.exists(source_path)):
					compile_source(source_path, to_object_path(source_path))

				if (path.exists(folder_path)):
					for source_file in listdir(folder_path):
						source_path = path.join(folder_path, source_file)

						compile_source(source_path, to_object_path(source_path))
		else:
			for module in build_config["modules"]:
				folder_path = path.join(build_path, module)
				source_path = (folder_path + ".cpp")

				if (path.exists(source_path)):
					object_path = to_object_path(source_path)

					if (path.getmtime(source_path) > path.getmtime(object_path)):
						compile_source(source_path, object_path)
				else:
					header_path = (folder_path + ".hpp")

					if (
						(not path.exists(binary_path)) or
						(path.getmtime(header_path) > path.getmtime(binary_path))
					):
						# The module header file has been altered so recompile all source files in it.
						needs_recompile = True

						if (path.exists(folder_path)):
							for source_file in listdir(folder_path):
								source_path = path.join(folder_path, source_file)

								compile_source(source_path, to_object_path(source_path))
					elif path.exists(folder_path):
						for source_file in listdir(folder_path):
							source_path = path.join(folder_path, source_file)
							object_path = to_object_path(source_path)

							if (
								(not path.exists(object_path)) or
								(path.getmtime(source_path) > path.getmtime(object_path))
							):
								# A module source file has been altered so recompile it.
								needs_recompile = True

								compile_source(source_path, object_path)

		if (needs_recompile):
			if (all((pid == 0) for pid in [pid.wait() for pid in compilation_process_ids])):
				target_type_linkers[target_type](name, build_config, object_paths)

	return needs_recompile

arg_parser = ArgumentParser(
	description = "Builds an Ona engine component and all of its dependencies."
)

arg_parser.add_argument("component", help = "Component to compile")

args = arg_parser.parse_args()
component = args.component

if (not build(component)):
	print("Nothing to be done")
