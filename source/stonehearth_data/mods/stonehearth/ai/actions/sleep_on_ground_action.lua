local event_service = stonehearth.events

local SleepOnGroundAction = class()
SleepOnGroundAction.name = 'sleep on the cold, hard, unforgiving ground'
SleepOnGroundAction.does = 'stonehearth:sleep'
SleepOnGroundAction.args = {}
SleepOnGroundAction.version = 2
SleepOnGroundAction.priority = 1

function SleepOnGroundAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
end

function SleepOnGroundAction:start_thinking(ai, entity)
   radiant.events.listen(entity, 'stonehearth:attribute_changed:sleepiness', self, self._sleepiness_changed)
end

function SleepOnGroundAction:stop_thinking(ai, entity)
   radiant.events.unlisten(entity, 'stonehearth:attribute_changed:sleepiness', self, self._sleepiness_changed)
end

function SleepOnGroundAction:_sleepiness_changed(e)
   if e.value >= 120 then
      self._ai:set_think_output()
   else
      self._ai:clear_think_output()
   end
end

function SleepOnGroundAction:run(ai, entity, bed, path)
   local drop_location = radiant.entities.get_world_grid_location(entity)
   ai:execute('stonehearth:drop_carrying', { location = drop_location })
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })
   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = 1})
   radiant.entities.set_attribute(entity, 'sleepiness', 0)
end

function SleepOnGroundAction:stop()
   radiant.entities.add_buff(self._entity, 'stonehearth:buffs:groggy');
   radiant.entities.unthink(self._entity, '/stonehearth/data/effects/thoughts/sleepy')

   if self._timer then
      stonehearth.calendar:remove_timer(self._timer)
   end
   -- xxx, localize
   local name = radiant.entities.get_display_name(self._entity)
   event_service:add_entry(name .. ' awakes groggy from sleeping on the cold, hard, unforgiving earth.', 'warning')
end

return SleepOnGroundAction

