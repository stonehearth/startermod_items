from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class EntityContainer(Component):
   children = dm.Map(dm.ObjectId(), EntityRef(), singular_name='child', insert='declare', remove='declare', iterate='define')
   _private_variables = """
   std::unordered_map<dm::ObjectId, dm::TracePtr> destroy_traces_;
   """

   _private = \
   """
   std::unordered_map<dm::ObjectId, dm::TracePtr>   destroy_traces_;
   """