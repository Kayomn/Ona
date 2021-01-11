#!/bin/python3

from sys import exit
from os import path, listdir, mkdir
from subprocess import PIPE, Popen, call
from concurrent import futures
import json

# Settings
common_flags = ["-fno-exceptions", "-std=c++17", "-I.", "-g"]
assets_path = "./assets"
output_path = "./output"
components_path = "./components"
engine_path = "./engine"
build_config_file_path = "./build.json"
compiler = "clang++"

if (not path.exists(build_config_file_path)):
	print("Error: could not find", build_config_file_path)
	exit(1)

with open(build_config_file_path) as build_config_file:
	build_config = json.load(build_config_file)
	components = dict(build_config["components"]) if ("components" in build_config) else {}
	output_file_path = path.join(output_path, "engine")

	# Validates the headers modification timestamps of `component` and any dependencies against
	# `modtime`, returning `True` if everything is up to date and `False` if a rebuild is necessary.
	def validate_component_headers(modtime: int, component: str, dependencies: list) -> bool:
		for dependency in dependencies:
			if (not validate_component_headers(modtime, dependency, components[dependency])):
				return False

		component_path = path.join(components_path, component)

		# Rebuild if the component exports header is newer than output file.
		export_file_path = path.join(component_path, "exports.hpp")

		if (path.exists(export_file_path) and (path.getmtime(export_file_path) > modtime)):
			return False

		# Rebuild if any header files of component are newer than output file.
		header_files_path = path.join(component_path, "header")

		if (path.exists(header_files_path)):
			for header_file_name in listdir(header_files_path):
				header_file_path = path.join(header_files_path, header_file_name)

				if (path.getmtime(header_file_path) > modtime):
					return False

		return True

	def compile_source(prefix: str, source_file_path: str, clean: bool) -> str:
		object_file_path = path.join(
			output_path,
			"%s.%s.o" % (prefix, path.splitext(path.split(source_file_path)[1])[0])
		)

		if (clean):
			print(source_file_path)

			return Popen(
				[compiler, source_file_path, ("-o" + object_file_path), "-c"] + common_flags,
				stdout = PIPE,
			).stdout.read()
		else:
			if (
				not path.exists(object_file_path) or
				(path.getmtime(source_file_path) > path.getmtime(object_file_path))
			):
				print(source_file_path)

				return Popen(
					[compiler, source_file_path, ("-o" + object_file_path), "-c"] + common_flags,
					stdout = PIPE,
				).stdout.read()


		return ""

	def check_compilation_futures(compilation_futures: list) -> None:
		for compilation_future in compilation_futures:
			error_message = compilation_future.result()

			if (len(error_message) != 0):
				print(error_message)

	# Builds `component` if necessary, returning `True` if a rebuild has occured, otherwise `False`.
	def build_component(component: str, dependencies: list) -> bool:
		compilation_futures = []
		has_rebuilt = False

		with futures.ThreadPoolExecutor() as executor:
			source_files_path = path.join(components_path, component, "source")

			if (path.exists(output_file_path)):
				output_file_modtime = path.getmtime(output_file_path)

				# Recursively check dependency headers and rebuild if any are newer than output
				# file.
				if (validate_component_headers(output_file_modtime, component, dependencies)):
					if (path.exists(source_files_path)):
						for file_name in listdir(source_files_path):
							source_file_path = path.join(source_files_path, file_name)

							if (path.getmtime(source_file_path) > output_file_modtime):
								compilation_futures.append(executor.submit(
									compile_source,
									component,
									source_file_path,
									True
								))

								has_rebuilt = True
			else:
				# Build fresh if an output file does not yet exist.
				if (path.exists(source_files_path)):
					for file_name in listdir(source_files_path):
						compilation_futures.append(executor.submit(
							compile_source,
							component,
							path.join(source_files_path, file_name),
							True
						))

						has_rebuilt = True

		check_compilation_futures(compilation_futures)

		return has_rebuilt

	requires_rebuild = False

	for component, depdendencies in components.items():
		requires_rebuild |= build_component(component, depdendencies)

	source_files_path = path.join(engine_path, "source")
	compilation_futures = []

	with futures.ThreadPoolExecutor() as executor:
		if (requires_rebuild):
			for file_name in listdir(source_files_path):
				source_file_path = path.join(source_files_path, file_name)

				compilation_futures.append(executor.submit(
					compile_source,
					"engine",
					source_file_path,
					True
				))
		else:
			for file_name in listdir(source_files_path):
				source_file_path = path.join(source_files_path, file_name)

				compilation_futures.append(executor.submit(
					compile_source,
					"engine",
					source_file_path,
					False
				))

		check_compilation_futures(compilation_futures)

		args = [compiler]

		for file_name in listdir(output_path):
			if (file_name.endswith(".o")):
				args.append(path.join(output_path, file_name))

		args.append("-o" + output_file_path)

		if ("libraries" in build_config):
			for library in build_config["libraries"]:
				args.append("-l" + library)

		call(args + common_flags)

