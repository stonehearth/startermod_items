import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
from ridl.om_types import *

class Sensor:
   name = "Sensor"

class SensorList(Component):
   sensors = dm.Map(std.string(), std.shared_ptr(Sensor), add=None, singular_name='sensor', accessor_value=std.weak_ptr(Sensor))

   add_sensor = ridl.Method(std.weak_ptr(Sensor),
                            ('name', std.string().const.ref),
                            ('radius', c.int()))
   _lua_includes = [
      "om/components/sensor.ridl.h"
   ]

