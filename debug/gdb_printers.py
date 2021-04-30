import gdb.printing

class StringPrinter:
	def __init__(self, val):
		self.text = val["buffer"]["static_"] if (val["size"] > 25) else val["buffer"]["dynamic"]

	def to_string(self):
		return self.text

printer = gdb.printing.RegexpCollectionPrettyPrinter("Ona")

printer.add_printer("String", "^Ona::String$", StringPrinter)

gdb.printing.register_pretty_printer(gdb.current_objfile(), printer, replace = True)
