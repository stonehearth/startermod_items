from ridl.om_types import *
import ridl.dm_types as dm
import ridl.ridl as ridl

class ModelLayer(dm.Record):
   name = 'ModelLayer'
   
class ModelVariants(Component):
   model_variants = dm.Set(std.shared_ptr(ModelLayer), singular_name='model_variant')

   _public = \
   """
   ModelLayerPtr GetModelVariant(std::string const& v) const;
   """
