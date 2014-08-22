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
   self._trap_added = false
   ai:set_think_output()
end

function SetBaitTrapAdjacent:run(ai, entity, args)
   local trapping_grounds = args.trapping_grounds

   if not trapping_grounds:is_valid() then
      ai:abort('cannot set trap because trapping grounds is destroyed')
   end

   self._trap = radiant.entities.create_entity(args.trap_uri)
   local picked_up = radiant.entities.pickup_item(entity, self._trap)
   assert(picked_up)

   ai:execute('stonehearth:drop_carrying_adjacent', {
      location = args.location
   })

   if trapping_grounds:is_valid() then
      local trap_component = self._trap:add_component('stonehearth:bait_trap')
      trap_component:arm()

      local trapping_grounds_component = trapping_grounds:add_component('stonehearth:trapping_grounds')
      trapping_grounds_component:add_trap(self._trap)

      self._trap_added = true
   end

   -- trap is large, so we need to get some separation distance
   -- could replace this by supporting stop distances in goto_location as we do with goto_entity
   ai:execute('stonehearth:bump_against_entity', {
      entity = self._trap,
      distance = 1.7
   })

   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })
end

function SetBaitTrapAdjacent:stop(ai, entity, args)
   -- clean up orhpaned trap if this action was interrupted or the trapping grounds was destroyed
   if self._trap and not self._trap_added then
      radiant.entities.destroy_entity(self._trap)
      self._trap = nil
   end
end

return SetBaitTrapAdjacent
