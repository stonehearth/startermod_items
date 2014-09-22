local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local SetBaitTrapAdjacent = class()

SetBaitTrapAdjacent.name = 'set bait trap adjacent'
SetBaitTrapAdjacent.does = 'stonehearth:trapping:set_bait_trap_adjacent'
SetBaitTrapAdjacent.args = {
   location = Point3,
   trap_uri = 'string',
   trapping_grounds = Entity
}
SetBaitTrapAdjacent.version = 2
SetBaitTrapAdjacent.priority = 1

function SetBaitTrapAdjacent:start_thinking(ai, entity, args)
   self._finished_arming = false
   self._trap_added = false

   local trapping_grounds_component = args.trapping_grounds:add_component('stonehearth:trapping_grounds')
   for id, trap in pairs(trapping_grounds_component:get_traps()) do
      if radiant.entities.get_world_grid_location(trap) == args.location then
         -- trap exists, but we didn't finish arming it
         self._trap_added = true
      end
   end

   ai:set_think_output()
end

function SetBaitTrapAdjacent:run(ai, entity, args)
   local trapping_grounds = args.trapping_grounds

   if not self._trap_added then
      self._trap = radiant.entities.create_entity(args.trap_uri)
      local picked_up = radiant.entities.pickup_item(entity, self._trap)
      assert(picked_up)

      ai:execute('stonehearth:drop_carrying_adjacent', {
         location = args.location
      })

      if trapping_grounds:is_valid() then
         local trapping_grounds_component = trapping_grounds:add_component('stonehearth:trapping_grounds')
         trapping_grounds_component:add_trap(self._trap)
         self._trap_added = true
      end
   end

   -- trap is large, so we need to get some separation distance
   -- could replace this by supporting stop distances in goto_location as we do with goto_entity
   ai:execute('stonehearth:bump_against_entity', {
      entity = self._trap,
      distance = 1.7
   })

   -- animation to add bait and arm the trap
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   if self._trap:is_valid() then
      local trap_component = self._trap:add_component('stonehearth:bait_trap')
      trap_component:arm()
      self._finished_arming = true
   end
end

function SetBaitTrapAdjacent:stop(ai, entity, args)
   if self._trap and self._trap:is_valid() then
      if self._trap_added then
         if self._finished_arming then
            -- success! do nothing
         else
            -- disarm it if we're interrupted while setting the bait
            -- we do this because we drop an armed model because it looks better
            local trap_component = self._trap:add_component('stonehearth:bait_trap')
            trap_component:trigger(nil)
         end
      else
         -- clean up orhpaned trap if we were interrupted before it was added to the trapping grounds
         radiant.entities.destroy_entity(self._trap)
         self._trap = nil
      end
   end
end

return SetBaitTrapAdjacent
