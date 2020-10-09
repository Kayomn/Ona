#!/bin/python3

from argparse import ArgumentParser
from os import path, listdir
from subprocess import call
from concurrent import futures
import json

processed_dependencies = []
common_d_flags = ["--gc"]

common_cpp_flags = [
	"-g",
	"--std=c++14",
	"-fno-exceptions",
	"-fno-rtti",
	"-fno-threadsafe-statics",
	"-I."
]

output_path = "output"
input_path = "ona"

required_properties = [
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
	module_config = load_build_config(module_path + ".json")

	for required_property in required_properties:
		if (not required_property in module_config):
			print("No", required_property, "specified in build.json for", name)
			exit(1)

	object_paths = []
	dependency_paths = []
	needs_recompile = False

	def to_object_path(source_path: str) -> str:
		path_nodes = []

		while (source_path):
			split = path.split(source_path)
			source_path = split[0]

			path_nodes.append(split[1])

		if (len(path_nodes)):
			path_nodes[0] = (path.splitext(path_nodes[0])[0] + ".o")

			path_nodes.reverse()

			object_path = path.join(output_path, ".".join(path_nodes))

			object_paths.append(object_path)

			return object_path

		return ""

	version_d_flags = []
	version_cpp_flags = []

	if ("versions" in module_config):
		for version in module_config["versions"]:
			version_d_flags.append("--d-version=" + version)
			version_cpp_flags.append("-D version_" + version)

	if ("dependencies" in module_config):
		for dependency in module_config["dependencies"]:
			if (not dependency in processed_dependencies):
				build_result, dependency_path = build(dependency)
				needs_recompile |= build_result

				dependency_paths.append(dependency_path)
				processed_dependencies.append(dependency)

	binary_file_path = path.join(output_path, name)

	print("Building", (name + "..."))

	# A re-compilation is needed if the module header is newer than the output binary.
	needs_recompile = (not path.exists(binary_file_path))

	with futures.ThreadPoolExecutor() as executor:
		def compile_d(source_path: str, object_path: str) -> int:
			print(source_path)

			return call(
				["ldc2", source_path, ("-of=" + object_path), "-c"] +
				version_d_flags +
				common_d_flags
			)

		def compile_cpp(source_path: str, object_path: str) -> int:
			print(source_path)

			return call(
				["clang++", source_path, ("-o" + object_path), "-c"] +
				version_cpp_flags +
				common_cpp_flags
			)

		compilation_futures = []

		if (path.exists(module_path)):
			if (needs_recompile):
				# A dependency has changed so re-compile the entire module.
				for file_name in listdir(module_path):
					if (file_name.endswith(".d")):
						file_path = path.join(module_path, file_name)

						compilation_futures.append(executor.submit(
							compile_d,
							file_path,
							to_object_path(file_path)
						))
					elif (file_name.endswith(".cpp")):
						file_path = path.join(module_path, file_name)

						compilation_futures.append(executor.submit(
							compile_cpp,
							file_path,
							to_object_path(file_path)
						))
			else:
				for file_name in listdir(module_path):
					if (file_name.endswith(".d")):
						file_path = path.join(module_path, file_name)
						object_path = to_object_path(file_path)

						if (
							(not path.exists(object_path)) or
							(path.getmtime(file_path) > path.getmtime(object_path))
						):
							compilation_futures.append(executor.submit(
								compile_d,
								file_path,
								object_path
							))

							needs_recompile = True
					elif (file_name.endswith(".cpp")):
						file_path = path.join(module_path, file_name)
						object_path = to_object_path(file_path)

						if (
							(not path.exists(object_path)) or
							(path.getmtime(file_path) > path.getmtime(object_path))
						):
							compilation_futures.append(executor.submit(
								compile_cpp,
								file_path,
								to_object_path(file_path)
							))

							needs_recompile = True

		for future in compilation_futures:
			exit_code = future.result()

			if (exit_code != 0):
				exit(exit_code)

		if (needs_recompile):
			target_type = module_config["targetType"]

			if (target_type == "shared-lib"):
				print("Linking shared library...")
			elif (target_type == "static-lib"):
				print("Linking static library...")

				binary_file_path += ".a"

				call(["ar", "rc", binary_file_path] + object_paths)
			elif (target_type == "executable"):
				print("Linking executable...")

				args = (
					["ldc2"] +
					object_paths +
					dependency_paths +
					["-of=" + binary_file_path] +
					common_d_flags
				)

				if "libraries" in module_config:
					for library in module_config["libraries"]:
						args.append("-L=-l" + library)

				call(args)
			else:
				print("Unknown target type: ", target_type)

	return needs_recompile, binary_file_path

arg_parser = ArgumentParser(
	description = "Builds an Ona engine component and all of its dependencies."
)

arg_parser.add_argument("component", help = "Component to compile")

args = arg_parser.parse_args()
component = args.component

if (not build(component)[0]):
	print("Nothing to be done")
