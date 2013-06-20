local PickupItemAction = class()

PickupItemAction.name = 'stonehearth.actions.pickup_item'
PickupItemAction.does = 'stonehearth.activities.pickup_item'
PickupItemAction.priority = 5
--TODO we need a scale to  describe relative importance

function PickupItemAction:run(ai, entity, item)
   radiant.check.is_entity(item)
   local carry_block = entity:get_component('carry_block') 

   if carry_block:is_carrying() then
      local carrying = carry_block:get_carrying()
      if carrying:get_id() ~= item:get_id() then
         ai:abort('cannot pick up another item while carrying one!')
      end
      return
   end
   
   --go stand adjacent to the item      
   local obj_location = radiant.entities.get_world_grid_location(item)
   if not radiant.entities.is_adjacent_to(entity, obj_location) then
      ai:execute('stonehearth.activities.goto_entity', item)
   end

   radiant.log.info("picking up item at %s", tostring(obj_location))
   
   radiant.entities.turn_to_face(entity, item)
   radiant.entities.pickup_item(entity, item)
   ai:execute('stonehearth.activities.run_effect', 'carry_pickup')
   --[[
   --TODO: replace with ai:execute('stonehearth.activities.run_effect,''carry_pickup')
   --It does not seem to be working right now, but this does:
   self._effect = radiant.effects.run_effect(entity, 'carry_pickup')
   ai:wait_until(function()
      return self._effect:finished()
   end)
   self._effect = nil
   --]]
end

return PickupItemAction