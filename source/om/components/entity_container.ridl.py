from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class EntityContainer(Component):
   children = dm.Map(dm.ObjectId(), EntityRef(), singular_name='child', add=None, remove='declare', iterate='define')
   add_child = ridl.Method(c.void(), ('child', EntityRef()))
   add_child_to_bone = ridl.Method(c.void(), ('child', EntityRef()), ('bone', std.string().const.ref))

   attached_items = dm.Map(dm.ObjectId(), EntityRef(), add=None, remove=None, iterate=None)

   _public = """
   void OnLoadObject(dm::SerializationType r) override;
   """

   _private = """
   void AddTrace(std::weak_ptr<Entity> c);
   void RemoveTrace(dm::ObjectId childId);
   void EntityContainer::AddChildToContainer(dm::Map<dm::ObjectId, EntityRef> &children, EntityRef c, std::string const& bone);
   std::unordered_map<dm::ObjectId, dm::TracePtr>  dtor_traces_;
   """
