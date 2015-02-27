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
   self._thinking = true

   self:create_listeners()
   self:_check_formation_location()
end

function HoldFormationAction:stop_thinking(ai, entity, args)
   self._thinking = false
   if not self._running then
      self:_destroy_listeners()
   end
end

function HoldFormationAction:start(ai, entity, args)
   self:create_listeners()
   self._running = true
   self._thought = radiant.entities.think(self._entity, self._thought_bubble, stonehearth.constants.think_priorities.PARTY_FORMATION)
end

function HoldFormationAction:stop(ai, entity, args)
   if self._thought then
      self._thought:destroy()
      self._thought = nil
   end
   self._running = false
   self:_destroy_listeners()
end

function HoldFormationAction:create_listeners()
   if not self._party_listener then
      self._party_listener = radiant.events.listen(self._party, 'stonehearth:party:banner_changed', self, self._check_formation_location)
   end
end

function HoldFormationAction:_destroy_listeners()
   if self._party_listener then
      self._party_listener:destroy()
      self._party_listener = nil
   end
end

function HoldFormationAction:_check_formation_location()
   local location
   local banner = self._party:get_active_banner()
   if banner then
      location = banner.location + self._party:get_formation_offset(self._entity)
   end

   if self._running then
      if location ~= self._location then
         self._ai:abort('formation location changed')
      end
      return
   end
   if self._thinking and location then
      self._thinking = false
      self._location = location
      self._thought_bubble = string.format('/stonehearth/data/effects/thoughts/party_%s', banner.type)
      self._ai:set_think_output({ location = location })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(HoldFormationAction)
         :execute('stonehearth:party:hold_formation_location', { location = ai.PREV.location  })
