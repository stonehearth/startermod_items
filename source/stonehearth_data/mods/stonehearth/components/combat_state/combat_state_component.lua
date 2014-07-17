local CombatStateComponent = class()

function CombatStateComponent:__init()
   -- attacks in progress are not saved, so neither are their assault events
   self._assault_events = {}
   self._assaulting = false

   -- cache of the available combat actions
   self._combat_actions = {}
end

function CombatStateComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.cooldowns = {}
      self._sv.panicking = false
      self._sv.stance = 'aggressive'
      self._sv.initialized = true

      if json then
         if json.stance ~= nil then
            self._sv.stance = json.stance
         end
      end
   end

   radiant.events.listen(self._entity, 'stonehearth:equipment_changed', self, self._on_equipment_changed)
end

function CombatStateComponent:destroy()
   radiant.events.unlisten(self._entity, 'stonehearth:equipment_changed', self, self._on_equipment_changed)
end

-- duration is in milliseconds at game speed 1
function CombatStateComponent:start_cooldown(name, duration)
   -- TODO: could set a trigger after expiration to remove old cooldowns
   self:_remove_expired_cooldowns()

   local now = radiant.gamestate.now()
   self._sv.cooldowns[name] = now + duration
   self.__saved_variables:mark_changed()
end

function CombatStateComponent:in_cooldown(name)
   return self:get_cooldown_end_time(name) ~= nil
end

-- returns nil if the cooldown does not exist or has expired
-- note: start time is part of cooldown, while the end time is not
function CombatStateComponent:get_cooldown_end_time(name)
   local cooldown_end_time = self._sv.cooldowns[name]

   if cooldown_end_time ~= nil then
      local now = radiant.gamestate.now()

      if now < cooldown_end_time then
         return cooldown_end_time
      end

      -- expired, clean up
      self._sv.cooldowns[name] = nil
   end

   return nil
end

function CombatStateComponent:add_assault_event(context)
   table.insert(self._assault_events, context)
   self.__saved_variables:mark_changed()
end

function CombatStateComponent:remove_assault_event(context)
   for index, value in pairs(self._assault_events) do
      if value == context then
         table.remove(self._assault_events, index)
         return
      end
   end
end

function CombatStateComponent:get_assault_events()
   return self._assault_events
end

function CombatStateComponent:get_assaulting()
   return self._assaulting
end

function CombatStateComponent:set_assaulting(assaulting)
   self._assaulting = assaulting
end

-- currently experimental code
function CombatStateComponent:get_primary_target()
   return self._sv.primary_target
end

function CombatStateComponent:set_primary_target(target)
   if target ~= self._sv.primary_target then
      self._sv.primary_target = target
      self.__saved_variables:mark_changed()
      radiant.events.trigger_async(self._entity, 'stonehearth:combat:primary_target_changed')
   end
end

-- probably have three stances: 'aggressive', 'defensive', and 'passive'
function CombatStateComponent:get_stance()
   return self._sv.stance
end

function CombatStateComponent:set_stance(stance)
   if stance ~= self._sv.stance then
      self._sv.stance = stance
      self.__saved_variables:mark_changed()
      radiant.events.trigger_async(self._entity, 'stonehearth:combat:stance_changed')
   end
end

function CombatStateComponent:panicking()
   return self._sv.panicking_from_id ~= nil
end

function CombatStateComponent:get_panicking_from()
   local threat = radiant.entities.get_entity(self._sv.panicking_from_id)
   return threat
end

function CombatStateComponent:set_panicking_from(threat)
   local threat_id = threat and threat:get_id()
   self._sv.panicking_from_id = threat_id
   self.__saved_variables:mark_changed()
   radiant.events.trigger_async(self._entity, 'stonehearth:combat:panic_changed')
end

function CombatStateComponent:get_combat_actions(action_type)
   local actions = self._combat_actions[action_type]

   if not actions then
      actions = self:_compile_combat_actions(action_type)
      self._combat_actions[action_type] = actions
   end

   return actions
end

-- combat actions can come from two sources:
--   1) something you are (a dragon has a tail attack)
--   2) something you have (a wand of lightning bolt)
function CombatStateComponent:_compile_combat_actions(action_type)
   -- get actions from entity description
   local actions = radiant.entities.get_entity_data(self._entity, action_type)
   actions = actions or {}

   local equipment_component = self._entity:get_component('stonehearth:equipment')

   -- get actions from all equipment
   if equipment_component then
      local items = equipment_component:get_all_items()
      for _, item in pairs(items) do
         local item_actions = radiant.entities.get_entity_data(item, action_type)
         if item_actions then
            for _, action in pairs(item_actions) do
               table.insert(actions, action)
            end
         end
      end
   end

   table.sort(actions,
      function (a, b)
         return a.priority > b.priority
      end
   )

   return actions
end

function CombatStateComponent:_on_equipment_changed()
   -- clear the combat actions cache
   self._combat_actions = {}
end

function CombatStateComponent:_remove_expired_cooldowns()
   local now = radiant.gamestate.now()
   local cooldowns = self._sv.cooldowns
   local changed = false

   -- safe to remove entries in a pairs loop: http://lua-users.org/wiki/TablesTutorial
   for name, cooldown_end_time in pairs(cooldowns) do
      if now >= cooldown_end_time then
         cooldowns[name] = nil
         changed = true
      end
   end

   if changed then
      self.__saved_variables:mark_changed()
   end
end

return CombatStateComponent
