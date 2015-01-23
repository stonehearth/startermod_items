local CarpenterClass = class()
local job_helper = require 'jobs.job_helper'

--- Public functions, required for all classes

function CarpenterClass:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self:restore()
end

function CarpenterClass:restore()
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      self:_create_xp_listeners()
   end

   self.__saved_variables:mark_changed()
end

function CarpenterClass:promote(json, talisman_entity)
   job_helper.promote(self._sv, json)

   local crafter_component = self._sv._entity:add_component("stonehearth:crafter", json.crafter) 
   if talisman_entity then
      crafter_component:setup_with_existing_workshop(talisman_entity)
   end

   self:_create_xp_listeners()
   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function CarpenterClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function CarpenterClass:is_max_level()
   return self._sv.is_max_level 
end

-- Returns all the data for all the levels
function CarpenterClass:get_level_data()
   return self._sv.level_data
end

-- We keep an index of perks we've unlocked for easy lookup
function CarpenterClass:unlock_perk(id)
   self._sv.attained_perks[id] = true
   self.__saved_variables:mark_changed()
end

-- Given the ID of a perk, find out if we have the perk. 
function CarpenterClass:has_perk(id)
   return self._sv.attained_perks[id]
end

--Returns true if this class participates in worker defense. False otherwise.
--Unless the class specifies in their _description, it's true by default.
function CarpenterClass:get_worker_defense_participation()
   return self._sv.worker_defense_participant
end


-- Call when it's time to level up in this class
function CarpenterClass:level_up()
   job_helper.level_up(self._sv)
   self.__saved_variables:mark_changed()
end

-- If the carpenter has a workshop, associate it with the given talisman
-- Usually called when the talisman is spawned because the carpenter has died or been demoted
function CarpenterClass:associate_entities_to_talisman(talisman_entity)
   self._sv._entity:get_component('stonehearth:crafter'):associate_talisman_with_workshop(talisman_entity)
end

-- Call when it's time to demote
function CarpenterClass:demote()
   self:_remove_xp_listeners()

   self._sv._entity:get_component('stonehearth:crafter'):demote()
   self._sv._entity:remove_component("stonehearth:crafter")
   
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

function CarpenterClass:set_fine_percentage(args)
   self._sv._entity:get_component('stonehearth:crafter'):set_fine_percentage(args.percent_chance)
end

function CarpenterClass:unset_fine_percentage(args)
   self._sv._entity:get_component('stonehearth:crafter'):set_fine_percentage(0)
end

--- Private functions

function CarpenterClass:_create_xp_listeners()
   self._on_craft_listener = radiant.events.listen(self._sv._entity, 'stonehearth:crafter:craft_item', self, self._on_craft)
end

function CarpenterClass:_remove_xp_listeners()
   self._on_craft_listener:destroy()
   self._on_craft_listener = nil
end

--When we've crafted an item, we get XP according to the min-crafter level of the item
--If there is no level specified it's a default and should get that amount
--TODO: some items should give extra bonuses if they're really cool
function CarpenterClass:_on_craft(args)
   local recipe_data = args.recipe_data
   local level_key = 'craft_level_0'
   if recipe_data.level_requirement then
      level_key = 'craft_level_' .. recipe_data.level_requirement
   end
   local exp = self._sv.xp_rewards[level_key]
   self._job_component:add_exp(exp)
end

return CarpenterClass