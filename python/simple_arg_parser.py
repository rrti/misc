import sys

class cl_arg:
	def __init__(self, arg_name, arg_keys, arg_val):
		assert(type(arg_name) == str)
		assert(len(arg_name) != 0)

		self.m_arg_name = arg_name
		self.m_arg_keys = arg_keys
		## infer arg-type from the default value
		## (call the actual ctor during parsing)
		self.m_arg_con = type(arg_val)
		self.m_arg_val = arg_val

	def __repr__(self):
		return ("<n=\"%s\"::t=%s::v=%s>" % (self.m_arg_name, self.m_arg_con, self.m_arg_val))

	def get_name(self): return (self.m_arg_name)
	def get_keys(self): return (self.m_arg_keys)
	def get_val(self): return (self.m_arg_val)

	def parse_val(self, raw_arg_val):
		try:
			self.m_arg_val = self.m_arg_con(raw_arg_val)
		except ValueError:
			## stick to the default value
			pass

		return (self.m_arg_val)

class cl_arg_parser:
	def __init__(self):
		self.m_wanted_args = {} ## key: arg ("--N": 0x1234)
		self.m_parsed_args = {} ## name: val ("num_iters": 1000)

	def add_cl_arg(self, cl_arg_inst):
		assert(isinstance(cl_arg_inst, cl_arg))

		for cl_arg_key in cl_arg_inst.get_keys():
			self.m_wanted_args[cl_arg_key] = cl_arg_inst

		## set the default argument value
		## (so we can always query for it)
		self.set_parsed_arg(cl_arg_inst.get_name(), cl_arg_inst.get_val())

	def get_parsed_args(self): return (self.m_parsed_args)
	def set_parsed_arg(self, k, v): self.m_parsed_args[k] = v

	def parse_arg(self, raw_arg_key, raw_arg_val, cl_arg):
		## arg should have a non-empty list of non-empty string keys
		## though in practice it does not matter if this is violated:
		##   if key is a string the " in " will match substrings
		##   if key is a list and list is empty this returns false
		##   if key is a list and list contains T's this returns false
		##
		## assert(type(cl_arg.get_keys()) == list)
		## assert(len(cl_arg.get_keys()) != 0)
		## assert(type((cl_arg.get_keys())[0]) == str)
		##
		assert(type(raw_arg_key) == str)
		assert(type(raw_arg_val) == str)

		if (not (raw_arg_key in cl_arg.get_keys())):
			return False

		## set the actual argument value
		self.set_parsed_arg(cl_arg.get_name(), cl_arg.parse_val(raw_arg_val))
		return True

	def parse_args(self, raw_args):
		## note: arguments can be provided in any order
		for i in xrange(len(raw_args) - 1):
			if (self.m_wanted_args.has_key(raw_args[i])):
				self.parse_arg(raw_args[i], raw_args[i + 1], self.m_wanted_args[raw_args[i]])

	def print_args(self):
		for cl_arg in self.m_wanted_args.items():
			print("\"%s\" --> %s" % (cl_arg[0], cl_arg[1]))



def main(argc, argv):
	arg_parser = cl_arg_parser()
	arg_parser.add_cl_arg(cl_arg("num_iters", ["--N"],   12345))
	arg_parser.add_cl_arg(cl_arg("min_delta", ["--D"],   0.001))
	arg_parser.add_cl_arg(cl_arg("use_cheat", ["--C"],   False))
	arg_parser.add_cl_arg(cl_arg("win_title", ["--W"], "Title"))
	arg_parser.parse_args(argv[1: ])

	print("[main] parsed_args=%s" % arg_parser.get_parsed_args())
	return 0

## allow use as a module
if (__name__ == "__main__"):
	sys.exit(main(len(sys.argv), sys.argv))

