#!/usr/bin/env python
import pydax
import time

def event_callback(udata):
  print udata

pydax.init("PyDAX")

new_type = (("Mem1", "BOOL", 10),
            ("Mem2", "BOOL", 1),
            ("Mem3", "BOOL", 3))

try:
    x = pydax.cdt_create("PyDAX_Type", new_type)
except:
    pass
#print hex(x)

pydax.add("PyBYTE", "BYTE", 10)
pydax.add("PySINT", "INT", 10)
pydax.add("PyINT", "INT", 10)
pydax.add("PyINT", "INT", 10)
pydax.add("PyBOOL", "BOOL", 10)

pydax.add("PyCDTTAG", "PyDAX_TYPE", 1)
eid = pydax.event_add("PyBYTE", 10, "change", 1, event_callback, "nope")
pydax.event_del(eid)
print pydax.event_add("PyBYTE", 5, "change", 1, event_callback, "yep")

for n in range(1000):
  print pydax.read("PyCDTTAG", 0)
  print pydax.event_wait(1000)
  #print pydax.event_poll()
  #time.sleep(1)
#print pydax.get("PyDAXTAG")
#print pydax.get(0)
#print pydax.get(1)
#print pydax.get(2)

#print pydax.cdt_get("PyDAX_Type")

#print pydax.read("PyBYTE", 0)
#print pydax.read("PyBOOL[0]", 1)
#print pydax.read("PyBOOL", 0)


pydax.free()
