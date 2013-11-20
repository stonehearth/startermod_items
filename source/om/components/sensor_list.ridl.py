import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
from ridl.om_types import *

class Sensor:
   name = "Sensor"

class SensorList(Component):
   sensors = dm.Map(std.string(), std.shared_ptr(Sensor), insert=None)

   add_sensor = ridl.Method(std.shared_ptr(Sensor),
                            ('name', std.string().const.ref),
                            ('radius', c.int()))

