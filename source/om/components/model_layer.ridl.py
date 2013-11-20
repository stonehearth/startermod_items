from ridl.om_types import *
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.ridl as ridl

class ModelLayer(dm.Record):
   layer_type = ridl.Enum('Layer',
      SKELETON    = 0,
      SKIN        = 1,
      CLOTHING    = 2,
      ARMOR       = 3,
      CLOAK       = 4,
      NUM_LAYERS  = 5
   )
   models = dm.Set(std.string())
   layer = dm.Boxed(layer_type)
   variants = dm.Boxed(std.string())

   _public = \
   """
   void Init(json::Node const& obj);
   """
   
   _includes = [
      #include "dm/dm_save_impl.h"
      #include "protocols/store.pb.h"
   ]
   _global_post = \
   """
IMPLEMENT_DM_ENUM(radiant::om::ModelLayer::Layer);
   """