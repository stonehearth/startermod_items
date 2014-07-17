local TownDefenseWatcher = class()

TownDefenseWatcher.name = 'town defense watcher'
TownDefenseWatcher.does = 'stonehearth:town_defense:town_defense_watcher'
TownDefenseWatcher.args = {}
TownDefenseWatcher.version = 2
TownDefenseWatcher.priority = 1

function TownDefenseWatcher:start_thinking(ai, entity, args)
   self._entity = entity
   self._ai = ai

   if not self:_check_stance() then
      self._listening = true
      radiant.events.listen(self._entity, 'stonehearth:combat:stance_changed', self, self._check_stance)
   end
end

function TownDefenseWatcher:stop_thinking(ai, entity, args)
   if self._listening then
      radiant.events.unlisten(self._entity, 'stonehearth:combat:stance_changed', self, self._check_stance)
      self._listening = false
   end
end

function TownDefenseWatcher:_check_stance()
   local stance = stonehearth.combat:get_stance(self._entity)
   if stance == 'aggressive' then
      self._ai:set_think_output()
      self._listening = false
      return radiant.events.UNLISTEN
   end
   return nil
end

return TownDefenseWatcher
