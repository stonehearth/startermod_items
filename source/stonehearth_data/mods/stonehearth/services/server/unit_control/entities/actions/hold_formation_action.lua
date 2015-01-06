local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HoldFormation = class()

HoldFormation.name = 'hold party formation'
HoldFormation.does = 'stonehearth:party:orders'
HoldFormation.args = {
   party = 'table',     -- technically a party controller..
}
HoldFormation.version = 2
HoldFormation.priority = stonehearth.constants.priorities.party.HOLD_PARTY_FORMATION

function HoldFormation:start_thinking(ai, entity, args)
   self._thinking = true
   self._ai = ai
   self._entity = entity
   self._party = args.party

   self._trace = radiant.entities.trace_grid_location(entity, 'hold formation')
                     :on_changed(function()
                           self:_check_position()
                        end)
   self._listener = radiant.events.listen(args.party, 'stonehearth:party:formation_changed', self, self._check_formation)

   self:_check_formation()
   self:_check_position()
end


function HoldFormation:stop_thinking()
   self._thinking = false
   self:_destroy_listeners()
end

function HoldFormation:_check_formation()
   self._formation_location = self._party:get_formation_location_for(self._entity)
end

function HoldFormation:_check_position()
   assert(self._thinking)

   if not self._formation_location then
      return
   end

   local location = radiant.entities.get_world_grid_location(self._entity)
   if location:distance_to(self._formation_location) <= 2 then
      return
   end
   self:_destroy_listeners()

   self._ai:set_think_output({
         party = self._party,
         location = self._formation_location,
      })
end

function HoldFormation:_destroy_listeners()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
   if self._listener then
      self._listener:destroy()
      self._listener = nil
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(HoldFormation)
         :execute('stonehearth:goto_location', { reason = 'guard target', location = ai.PREV.location  })
