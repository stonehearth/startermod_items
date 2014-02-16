from ridl.om_types import *
import ridl.dm_types as dm
import ridl.ridl as ridl
import ridl.std_types as std

class ModelLayer(dm.Record):
   name = 'ModelLayer'
   
class ModelVariants(Component):
   variants = dm.Map(std.string(), std.shared_ptr(ModelLayer), singular_name='variant', add=None, remove='define', iterate='define')
   add_variant = ridl.Method(std.shared_ptr(ModelLayer), ('name', std.string().const.ref))

   _includes = [
      "om/components/model_layer.ridl.h",
   ]

