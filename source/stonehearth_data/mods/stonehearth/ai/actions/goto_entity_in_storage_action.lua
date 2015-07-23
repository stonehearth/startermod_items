local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local GotoEntityInStorage = class()

GotoEntityInStorage.name = 'go to entity in storage'
GotoEntityInStorage.does = 'stonehearth:goto_entity_in_storage'
GotoEntityInStorage.args = {
   entity = Entity,
   stop_distance = {
      type = 'number',
      default = 0
   },
}
GotoEntityInStorage.think_output = {
   point_of_interest = Point3
}
GotoEntityInStorage.version = 2
GotoEntityInStorage.priority = 1

function GotoEntityInStorage:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying then
      return
   end
   local player_id = radiant.entities.get_player_id_from_entity(entity)
   local item = radiant.entities.get_iconic_form(args.entity) or args.entity

   local storage = stonehearth.inventory:get_inventory(player_id):public_container_for(item)
   if storage then
      local parent = radiant.entities.get_parent(item)
      local use_container = not parent or parent == storage
      local pickup_target = use_container and storage or item

      ai:set_think_output({
         pickup_target = pickup_target,
      })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(GotoEntityInStorage)
         :execute('stonehearth:goto_entity', {
            entity = ai.PREV.pickup_target,
            stop_distance = ai.ARGS.stop_distance,
         })
         :set_think_output({
            point_of_interest = ai.PREV.point_of_interest
         })
