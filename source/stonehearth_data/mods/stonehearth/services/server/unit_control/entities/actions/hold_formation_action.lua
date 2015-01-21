local HoldFormationAction = class()

HoldFormationAction.name = 'hold party formation'
HoldFormationAction.does = 'stonehearth:party:hold_formation'
HoldFormationAction.args = {
   party = 'table',     -- technically a party controller..
}
HoldFormationAction.version = 2
HoldFormationAction.priority = stonehearth.constants.priorities.party.HOLD_FORMATION

function HoldFormationAction:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self._party = args.party
   self._town = stonehearth.town:get_town(entity)
   if not self._town then
      return
   end

   self:create_listeners()
   self:_check_formation_location()
end

function HoldFormationAction:stop_thinking(ai, entity, args)
   if not self._running then
      self:_destroy_listeners()
   end
end

function HoldFormationAction:start(ai, entity, args)
   self:create_listeners()
   self._running = true
end

function HoldFormationAction:stop(ai, entity, args)
   self._running = false
   self:_destroy_listeners()
end

function HoldFormationAction:create_listeners()
   if not self._town_listener then
      self._town_listener = radiant.events.listen(self._town, 'stonehearth:town_defense_mode_changed', self, self._check_formation_location)
   end
   if not self._party_listener then
      self._party_listener = radiant.events.listen(self._party, 'stonehearth:party:banner_changed', self, self._check_formation_location)
   end
end

function HoldFormationAction:_destroy_listeners()
   if self._town_listener then
      self._town_listener:destroy()
      self._town_listener = nil
   end
   if self._party_listener then
      self._party_listener:destroy()
      self._party_listener = nil
   end
end

function HoldFormationAction:_check_formation_location()
   local location
   if self._town:worker_combat_enabled() then
      location = self._party:get_banner_location('defend')
   end
   if not location then
      location = self._party:get_banner_location('attack')
   end

   if not location then
      return
   end

   location = location + self._party:get_formation_offset(self._entity)
   if location == self._location then
      return
   end

   if self._running then
      self._ai:abort('formation location changed')
   else
      self._ai:set_think_output({ location = location })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(HoldFormationAction)
         :execute('stonehearth:party:hold_formation_location', { location = ai.PREV.location  })
