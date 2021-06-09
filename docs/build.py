import os
import shutil

output_dir_path = "./output"
source_dir_path = "./source"

if (os.path.exists(output_dir_path)):
	for file in os.listdir(output_dir_path):
		os.remove(os.path.join(output_dir_path, file))
else:
	os.mkdir(output_dir_path)

for file in os.listdir(source_dir_path):
	shutil.copy(os.path.join(source_dir_path, file), output_dir_path)

source_paths = ["../engine/source"]

for source_path in source_paths:
	os.system(str(" ").join([
		"dub run adrdox",
		"--",
		source_path,
		"--skeleton ./output/skeleton.html",
		"-i",
		"--copy-standard-files=false",
		"--header-title=\"Ona Docs\" --header-link=\"https://bing.com\"",
		"-o ./output"
	]))

