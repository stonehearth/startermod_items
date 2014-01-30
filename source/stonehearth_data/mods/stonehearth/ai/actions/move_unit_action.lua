local MoveUnitAction = class()

MoveUnitAction.name = 'move unit'
MoveUnitAction.does = 'stonehearth:top'
MoveUnitAction.version = 1
MoveUnitAction.priority = 0

function MoveUnitAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._ai = ai
   
   radiant.events.listen(entity, 'stonehearth:move_unit', self, self.on_move_unit)
end

function MoveUnitAction:on_move_unit(e)
   if self._running then
      self._ai:abort()
   end

   self._location = e.location
   self._ai:set_action_priority(self, 1000)
end


function MoveUnitAction:run(ai, entity)
   self._running = true
   ai:execute('stonehearth:goto_location', self._location)
end

function MoveUnitAction:stop(ai, entity)
   self._running = false
   self._ai:set_action_priority(self, 0)
end

return MoveUnitAction