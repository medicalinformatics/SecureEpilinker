import pysel

spec = pysel.FieldSpec("name", 0.5, 0, "dice", "bitmask", 16) # weight=1
epicfg = pysel.EpilinkConfig({"name": spec}, [], 0.7, 0.9)
cfg = pysel.CircuitConfig(epicfg)

res = pysel.epilink_int({"name": bytes([0xff, 0xff]) }, {"name": [ None, bytes([0x7f, 0xf7]) ]}, cfg)
