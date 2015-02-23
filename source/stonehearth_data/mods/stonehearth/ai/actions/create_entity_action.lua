local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local CreateEntity = class()

CreateEntity.name = 'create entity'
CreateEntity.does = 'stonehearth:create_entity'
CreateEntity.args = {
   location = Point3,
   uri = {
      type = 'table',
      default = stonehearth.ai.NIL,
   },
   options = {
      type = 'table',
      default = {},
   }
}
CreateEntity.think_output = {
   entity = Entity   -- the proxy entity
}
CreateEntity.version = 2
CreateEntity.priority = 1

function CreateEntity:start_thinking(ai, entity, args)
   -- if we already have a proxy entity, someone has screwed us by making us
   -- thinking twice without an intervening call to stop_thinking()
   assert(not self._created_entity)

   self._using_proxy_entity = false
   self._created_entity = radiant.entities.create_entity(args.uri, args.options)

   -- do not use radiant.terrain.place_entity, here.  that will put the entity on the terrain,
   -- which may not be the *exact* location passed in, which may screw some people up
   -- downstream
   radiant.entities.add_child(radiant._root_entity, self._created_entity, args.location)

   ai:set_think_output({ entity = self._created_entity })
end

function CreateEntity:stop_thinking()
   -- if we need to keep the proxy entity around, start() will have been called before
   -- stop_thinking() to let us know.  if we don't need it, nuke it now.
   if not self._using_proxy_entity then
      self:_destroy_proxy_entity()
   end
end

function CreateEntity:run()
   assert(self._using_proxy_entity and self._created_entity)
end

function CreateEntity:start()
   -- set a flag to keep the proxy entity around for the duration of run
   self._using_proxy_entity = true
end

function CreateEntity:stop()
   self:_destroy_proxy_entity()
end

function CreateEntity:destroy()
   self:_destroy_proxy_entity()
end

function CreateEntity:_destroy_proxy_entity()
   if self._created_entity then
      radiant.entities.destroy_entity(self._created_entity)
      self._created_entity = nil
      self._using_proxy_entity = false
   end
end

return CreateEntity
