local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local CreateProxyEntity = class()

CreateProxyEntity.name = 'create proxy entity'
CreateProxyEntity.does = 'stonehearth:create_proxy_entity'
CreateProxyEntity.args = {
   location = Point3,
   use_default_adjacent_region = {
      type = 'boolean',
      default = false,
   }
}
CreateProxyEntity.think_output = {
   entity = Entity   -- the proxy entity
}
CreateProxyEntity.version = 2
CreateProxyEntity.priority = 1

function CreateProxyEntity:start_thinking(ai, entity, args)
   -- if we already have a proxy entity, someone has screwed us by making us
   -- thinking twice without an intervening call to stop_thinking()
   assert(not self._proxy_entity)

   self._using_proxy_entity = false

   self._proxy_entity = radiant.entities.create_proxy_entity(args.use_default_adjacent_region)

   radiant.terrain.place_entity(self._proxy_entity, args.location)

   ai:set_think_output({ entity = self._proxy_entity })
end

function CreateProxyEntity:stop_thinking()
   -- if we need to keep the proxy entity around, start() will have been called before
   -- stop_thinking() to let us know.  if we don't need it, nuke it now.
   if not self._using_proxy_entity then
      self:_destroy_proxy_entity()
   end
end

function CreateProxyEntity:run()
   assert(self._using_proxy_entity and self._proxy_entity)
end

function CreateProxyEntity:start()
   -- set a flag to keep the proxy entity around for the duration of run
   self._using_proxy_entity = true
end

function CreateProxyEntity:stop()
   self:_destroy_proxy_entity()
end

function CreateProxyEntity:destroy()
   self:_destroy_proxy_entity()
end

function CreateProxyEntity:_destroy_proxy_entity()
   if self._proxy_entity then
      radiant.entities.destroy_entity(self._proxy_entity)
      self._proxy_entity = nil
      self._using_proxy_entity = false
   end
end

return CreateProxyEntity
