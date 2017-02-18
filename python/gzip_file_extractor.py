import os
import platform
import sys

import gzip
import tarfile

print(os.name)
print(os.getcwd())
print(platform.system(), platform.release())



def write_gzip_file(gzip_file_name):
	assert(gzip_file_name.endswith(".gz"))

	f = gzip.open(gzip_file_name, "wb")
	f.write(data)
	f = f.close()

def read_gzip_file(gzip_file_name):
	assert(gzip_file_name.endswith(".gz"))

	f = gzip.GzipFile(gzip_file_name)
	s = zfile.read()
	f = f.close()



def extract_tarred_gzip_file(inp_tgz_file_name, out_t_file_name):
	assert(gzip_file_name.endswith(".tar.gz"))

	inp_file = gzip.open(inp_tgz_file_name, "rb")
	out_file = open(out_t_file_name, 'w')
	out_file.write(inp_tgz_file.read())
	inp_file.close()
	out_file.close()


def write_file(file_name, file_bytes):
	f = open(file_name, "w")
	f.write("%s" % file_bytes)
	f = f.close()

def extract_tar_file(tar_file_name, extract_dir_name = ".", extract_all = False):
	tar_file = tarfile.open(tar_file_name, "r:gz")

	if (not extract_all):
		## member_infos = tar_file.getmembers()
		member_names = tar_file.getnames()

		for member_name in member_names:
			if (os.path.isdir(member_name)):
				continue

			member = tar_file.getmember(member_name)
			handle = tar_file.extractfile(member)
			write_file(member_name, handle.read())
			handle.close()

		## same as "for member in tar_file.getmembers():"
		##   for member in tar_file: tar_file.extract(member)
		##
		## sequential member access pattern
		##   while (member != None): tar_file.extractfile(member); member = tar_file.next()
	else:
		tar_file.extractall(extract_dir_name)

	tar_file.close()

def extract_files(extract_dir_name):
	for fname in os.listdir(extract_dir_name):
		if (fname.endswith(".tar.gz")):
			extract_tar_file(fname, extract_dir_name)

def main(argc, argv):
	if (argc <= 1):
		extract_files(os.getcwd())
	else:
		extract_files(argv[1])

	return 0

sys.exit(main(len(sys.argv), sys.argv))

