import gdb.printing
import sys
sys.path.append('.')
import gdb_sel
gdb.printing.register_pretty_printer(
    gdb.current_objfile(),
    gdb_sel.build_pretty_printer())
