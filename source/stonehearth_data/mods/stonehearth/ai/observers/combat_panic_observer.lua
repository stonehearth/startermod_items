-- @title stonehearth:combat_panic_observer
-- @book reference
-- @section observer

local constants = require 'constants'
local log = radiant.log.create_logger('combat')

--[[ @markdown
The CombatPanicObserver is declared in the outfits of all non-combat classes. It determines when people should panic in combat. 
Whenever that person gets hit, the observer checks to see if their hitpoints are below their panic threshold. 
The panic threshold is set either in their eqipment, or defaults to 0.5, or 50%. 
If their health/max_health is <= threshold, the stonehearth.combat service will record that the entity is panicking from the person who lowered their HP to below the threshhold. 
If a person has a "panicking from" value that is not nil, they will run the panic action. 
The panic action usually involves either running from the entity or cowering, if there is nowhere to run to. 
]]--


local CombatPanicObserver = class()

-- On initialize the panic threshold from equipment, and listen to the combat:battery event
function CombatPanicObserver:initialize(entity)
   self._sv._entity = entity
end

function CombatPanicObserver:restore()
end

function CombatPanicObserver:activate()
   self._panic_threshold = self:_get_panic_threshold()

   self._battery_listener = radiant.events.listen(self._sv._entity, 'stonehearth:combat:battery', self, self._on_battery)
end

-- On destroy, remove the combat:battery listener 
function CombatPanicObserver:destroy()
   self._battery_listener:destroy()
   self._battery_listener = nil
end

-- Whenever this entity is attacked, compare health/max_health to the panic threshold. 
-- This equation just uses health right now, but this could depend on other attributes as well
function CombatPanicObserver:_on_battery(context)
   local health = radiant.entities.get_attribute(self._sv._entity, 'health')
   local max_health = radiant.entities.get_attribute(self._sv._entity, 'max_health')

   if health / max_health <= self._panic_threshold then
      stonehearth.combat:set_panicking_from(self._sv._entity, context.attacker)
   end
end

-- Get the panic threshold from the equipment the entity is holding. If nothing, set the default to 0.25
function CombatPanicObserver:_get_panic_threshold()
   local equipment_component = self._sv._entity:get_component('stonehearth:equipment')
   local data

   -- check if any items override the threshold and return the first found
   if equipment_component ~= nil then
      local items = equipment_component:get_all_items()
      for _, item in pairs(items) do
         data = radiant.entities.get_entity_data(item, 'stonehearth:panic_threshold')
         if data then
            return data
         end
      end
   end

   -- check if the threshold exists on the entity itself
   data = radiant.entities.get_entity_data(self._sv._entity, 'stonehearth:panic_threshold')
   if data then
      return data
   end

   -- return the default if none found
   return constants.combat.DEFAULT_PANIC_THRESHOLD
end

return CombatPanicObserver
