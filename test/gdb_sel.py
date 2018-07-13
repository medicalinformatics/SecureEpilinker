import gdb.printing
import numpy as np

def vec_at(vec, idx):
    return (vec['_M_impl']['_M_start']+idx).dereference()

class sharePrinter:
    """print an aby share object"""

    def __init__(self, obj):
        self.obj = obj

    def evalstr(self):
        return "*(({}*){})".format(str(self.obj.type), str(self.obj.address))

    def call_method(self, name, args):
        """call method on self, args are joined with ','"""
        return gdb.parse_and_eval("({}).{}({})".format(self.evalstr(),
            name, ','.join(args)))

    def to_string(self):
        obj = self.obj
        addr = obj.address
        nvals = int(self.call_method("get_nvals",[]))
        bitlen = int(self.call_method("get_bitlength",[]))
        print("nvals ", nvals)
        wires = self.obj["m_ngateids"]#.cast(gdb.lookup_type("uint32_t").array(nvals))
        circ = self.obj["m_ccirc"].dereference()
        def gate(id):
            # return circ["m_pGates"].cast(gdb.lookup_type("GATE").array(400000))[int(id)]
            return (circ["m_pGates"]+int(id)).dereference()
        print("wires={}\n".format(wires))
        wireunits = (nvals + 63)//64
        wirebytes = (nvals + 7)//8
        wire0 = vec_at(wires, 0)
        wireids = [vec_at(wires, idx) for idx in range(bitlen)]
        wirevalptrs = [gate(id)['gs']['ishare']['inval'].cast(gdb.lookup_type("uint8_t").pointer()) for id in wireids]
        def valarray(ptr):
            return [int((ptr+i).dereference()) for i in range(wirebytes)]
        vals = np.array([valarray(ptr) for ptr in wirevalptrs], dtype=np.uint8)
        ubits = np.unpackbits(vals, axis=1)[::-1,:]
        pbits = np.packbits(ubits, axis=0)
        #TODO remove padding from pbits, invert oder
        return str(pbits)

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("secure-epiliner")
    pp.add_printer('share', '^share$', sharePrinter)
    return pp
