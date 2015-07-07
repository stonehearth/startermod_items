local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local CreateEntity = class()

CreateEntity.name = 'create entity'
CreateEntity.does = 'stonehearth:create_entity'
CreateEntity.args = {
   location = Point3,
   rotation = {
      type = 'number',
      default = 0,
   },
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
   assert(not self._new_entity)

   self._using_proxy_entity = false
   self._new_entity = radiant.entities.create_entity(args.uri, args.options)

   local reference_entity = args.options.copy_adjacent_from
   if reference_entity then
      self:_propagate_adjacent(reference_entity, self._new_entity)
   end

   radiant.entities.turn_to(self._new_entity, args.rotation)

   -- Do not use radiant.terrain.place_entity here. That may move the entity to a 'standable'
   -- location which may not be the exact location desired.
   radiant.entities.add_child(radiant._root_entity, self._new_entity, args.location)

   ai:set_think_output({ entity = self._new_entity })
end

function CreateEntity:stop_thinking()
   -- if we need to keep the proxy entity around, start() will have been called before
   -- stop_thinking() to let us know.  if we don't need it, nuke it now.
   if not self._using_proxy_entity then
      self:_destroy_proxy_entity()
   end
end

function CreateEntity:run()
   assert(self._using_proxy_entity and self._new_entity)
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
   if self._new_entity then
      radiant.entities.destroy_entity(self._new_entity)
      self._new_entity = nil
      self._using_proxy_entity = false
   end
end

-- sets all the appropriate values to mirror the adjacent calculation from source to target
function CreateEntity:_propagate_adjacent(source, target)
   self:_copy_region_flags(source, target)
   self:_propagate_destination(source, target)
end

function CreateEntity:_copy_region_flags(source, target)
   local source_mob = source:add_component('mob')
   local target_mob = target:add_component('mob')

   target_mob:set_region_origin(source_mob:get_region_origin())
   target_mob:set_align_to_grid_flags(source_mob:get_align_to_grid_flags())
   target_mob:set_allow_vertical_adjacent(source_mob:get_allow_vertical_adjacent())
end

function CreateEntity:_propagate_destination(source, target)
   local boxed_destination_region = nil
   local boxed_adjacent_region = nil

   local source_destination = source:get_component('destination')
   if source_destination then
      boxed_destination_region = source_destination:get_region()
      boxed_adjacent_region = source_destination:get_adjacent()
   else
      local source_collision_shape = source:get_component('region_collision_shape')
      local region_collision_type = source_collision_shape and source_collision_shape:get_region_collision_type()
      if region_collision_type == _radiant.om.RegionCollisionShape.SOLID then
         -- use the collision region as the source of the adjacent if the destination region doesn't exist
         -- see MovementHelpers::GetRegionAdjacentToEntityInternal
         -- don't use it as a collision region, because it would be solid
         boxed_destination_region = source_collision_shape:get_region()
      end
   end

   if boxed_destination_region then
      local target_destination = target:add_component('destination')
      target_destination:set_region(boxed_destination_region)

      if boxed_adjacent_region then
         target_destination:set_adjacent(boxed_adjacent_region)
      else
         target_destination:set_auto_update_adjacent(true)
      end
   end
end

return CreateEntity
