#!/bin/python3

from argparse import ArgumentParser
from os import path, listdir
from subprocess import call
from threading import Thread
import json

processed_dependencies = []
common_flags = ["-g", "-fno-exceptions", "-std=c++20", "-I."]
output_path = "output"
input_path = "ona"

required_properties = [
	"language",
	"targetType"
]

def build(name: str) -> (bool, str):
	def load_build_config(file_path: str) -> dict:
		if (not path.exists(file_path)):
			print("No build.json exists for", name)
			exit(1)

		with open(file_path) as file:
			return json.load(file)

	module_path = path.join(input_path, name)
	build_config = load_build_config(module_path + ".json")

	for required_property in required_properties:
		if (not required_property in build_config):
			print("No", required_property, "specified in build.json for", name)
			exit(1)

	compilation_workers = []
	object_paths = []
	dependency_paths = []
	needs_recompile = False

	def compile_source(source_path: str, object_path: str) -> None:
		def worker_process() -> None:
			print(source_path)

			if (call(["clang++", source_path, ("-o" + object_path), "-c"] + common_flags) != 0):
				exit(1)

		worker = Thread(target = worker_process)

		compilation_workers.append(worker)
		worker.start()

	def to_object_path(source_path: str) -> str:
		path_nodes = []

		while (source_path):
			split = path.split(source_path)
			source_path = split[0]

			path_nodes.append(split[1])

		if (len(path_nodes)):
			path_nodes[0] = (path.splitext(path_nodes[0])[0] + ".o")

			# Messy hack to remove "source" folder from qualified path name.
			path_nodes.pop(1)
			path_nodes.reverse()

			object_path = path.join(output_path, ".".join(path_nodes))

			object_paths.append(object_path)

			return object_path

		return ""

	if ("dependencies" in build_config):
		for dependency in build_config["dependencies"]:
			if (not dependency in processed_dependencies):
				build_result, dependency_path = build(dependency)
				needs_recompile |= build_result

				dependency_paths.append(dependency_path)
				processed_dependencies.append(dependency)

	target_type = build_config["targetType"]
	binary_path = path.join(output_path, name)
	link = None

	if (target_type == "static-lib"):
		def link_static_lib() -> None:
			print("Linking", name, "static library...")
			call(["llvm-ar", "rc", binary_path] + object_paths)

		binary_path += ".a"
		link = link_static_lib
	elif (target_type == "executable"):
		def link_executable() -> None:
			print("Linking", name, "executable...")

			args = (
				["clang++"] +
				object_paths +
				dependency_paths +
				["-o" + path.join(output_path, name)] +
				common_flags
			)

			if ("libraries" in build_config):
				for library in build_config["libraries"]:
					args.append("-l" + library)

			call(args)

		link = link_executable
	elif (target_type == "shared-lib"):
		def link_shared_lib() -> None:
			print("Linking", name, "shared library...")
			# TODO: Implement shared object linking support.

		binary_path += ".so"
		link = link_shared_lib
	else:
		print("Invalid target type specified in module config for", name)
		exit(1)

	print("Building", (name + "..."))

	header_path = path.join(module_path, "header.hpp")

	# A re-compilation is needed if the module header is newer than the output binary.
	needs_recompile |= (
		not path.exists(binary_path) or
		(path.getmtime(header_path) > path.getmtime(binary_path))
	)

	source_path = path.join(module_path, "source")

	if (path.exists(source_path)):
		if (needs_recompile):
			# A dependency has changed so re-compile the entire module.
			for file_name in listdir(source_path):
				file_path = path.join(source_path, file_name)

				compile_source(file_path, to_object_path(file_path))
		else:
			for file_name in listdir(source_path):
				file_path = path.join(source_path, file_name)
				object_path = to_object_path(file_path)

				if (
					(not path.exists(object_path)) or
					(path.getmtime(file_path) > path.getmtime(object_path))
				):
					compile_source(file_path, object_path)

					needs_recompile = True

	if (needs_recompile):
		for worker in compilation_workers:
			worker.join()

		link()

	return needs_recompile, binary_path

arg_parser = ArgumentParser(
	description = "Builds an Ona engine component and all of its dependencies."
)

arg_parser.add_argument("component", help = "Component to compile")

args = arg_parser.parse_args()
component = args.component

if (not build(component)[0]):
	print("Nothing to be done")
