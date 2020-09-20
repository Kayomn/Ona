#!/bin/python3

from argparse import ArgumentParser
from os import path, listdir
from subprocess import Popen, call
import json

common_flags = ["-g", "-fno-exceptions", "-std=c++17", "-I./source"]
output_path = "output"
input_path = "source"

def to_object_path(source_path: str) -> str:
	path_nodes = []

	while (source_path):
		split = path.split(source_path)
		source_path = split[0]

		path_nodes.append(split[1])

	if (len(path_nodes)):
		path_nodes[0] = (path.splitext(path_nodes[0])[0] + ".o")

		path_nodes.reverse()

		if (path_nodes[0] == "modules"):
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
		for dependency in module_config["dependencies"]:
			args.append(path.join(output_path, (dependency + ".a")))

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

def build(name: str) -> bool:
	build_path = path.join(input_path, name)

	with open(path.join(build_path, "build.json")) as build_file:
		build_config = json.load(build_file)

		if ("modules" in build_config):
			def compile_module(source_path: str, header_path: str) -> bool:
				object_path = to_object_path(source_path)

				if (not object_path):
					print("Invalid object path")
					exit(1)

				recompile = False

				if (path.exists(object_path)):
					object_modtime = path.getmtime(object_path)

					if (path.exists(header_path) and (path.getmtime(header_path) > object_modtime)):
						recompile = True

					if (path.getmtime(source_path) > object_modtime):
						recompile = True
				else:
					recompile = True

				object_paths.append(object_path)

				if (recompile):
					print(source_path)

					compilation_process_ids.append(Popen([
						"clang++",
						source_path,
						("-o" + object_path),
						"-c"
					] + common_flags))

				return recompile

			compilation_process_ids = []
			object_paths = []
			has_recompiled = False

			if ("dependencies" in build_config):
				for dependency in build_config["dependencies"]:
					if (build(dependency)):
						has_recompiled = True

			print("Building", (name + "..."))

			for module in build_config["modules"]:
				module_path = path.join(build_path, module)
				header_path = (module_path + ".hpp")

				if (path.isdir(module_path) and path.isfile(header_path)):
					for source_file in listdir(module_path):
						if (compile_module(path.join(module_path, source_file), header_path)):
							has_recompiled = True
				else:
					module_path += ".cpp"

					if (path.isfile(module_path) and compile_module(module_path, header_path)):
						has_recompiled = True

			if (has_recompiled):
				if (all((pid == 0) for pid in [pid.wait() for pid in compilation_process_ids])):
					if ("targetType" in build_config):
						target_type = build_config["targetType"]

						if (target_type in target_type_linkers):
							target_type_linkers[target_type](name, build_config, object_paths)
						else:
							print("Invalid target type specified")
							exit(1)
					else:
						link_executable(name, build_config, object_paths)

			return has_recompiled

	return False


arg_parser = ArgumentParser(
	description = "Builds an Ona engine component and all of its dependencies."
)

arg_parser.add_argument("component", help = "Component to compile")

args = arg_parser.parse_args()

build(args.component)
