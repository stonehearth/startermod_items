from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class EntityContainer(Component):
   children = dm.Map(dm.ObjectId(), EntityRef(), singular_name='child', add=None, remove='declare', iterate='define')
   add_child = ridl.Method(c.void(), ('child', EntityRef()))

   _public = """
   void OnLoadObject(dm::SerializationType r) override;
   """

   _private = """
   void AddTrace(std::weak_ptr<Entity> c);
   void RemoveTrace(dm::ObjectId childId);
   std::unordered_map<dm::ObjectId, dm::TracePtr>  dtor_traces_;
   """
