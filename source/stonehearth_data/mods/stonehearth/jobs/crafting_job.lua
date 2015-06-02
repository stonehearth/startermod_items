-- Base class for all crafting jobs.
local constants = require 'constants'
local job_helper = require 'jobs.job_helper'

local CrafingJob = class()

--- Public functions, required for all classes
function CrafingJob:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self._sv.num_crafted = {}
   self:restore()
end

function CrafingJob:restore()
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      self:_create_xp_listeners()
   end

   self.__saved_variables:mark_changed()
end

function CrafingJob:promote(json, options)
   local talisman_entity = options and options.talisman
   
   job_helper.promote(self._sv, json)

   local crafter_component = self._sv._entity:add_component("stonehearth:crafter", json.crafter) 
   if talisman_entity then
      crafter_component:setup_with_existing_workshop(talisman_entity)
   end

   self:_create_xp_listeners()
   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function CrafingJob:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function CrafingJob:is_max_level()
   return self._sv.is_max_level 
end

-- Returns all the data for all the levels
function CrafingJob:get_level_data()
   return self._sv.level_data
end

-- We keep an index of perks we've unlocked for easy lookup
function CrafingJob:unlock_perk(id)
   self._sv.attained_perks[id] = true
   self.__saved_variables:mark_changed()
end

-- Given the ID of a perk, find out if we have the perk. 
function CrafingJob:has_perk(id)
   return self._sv.attained_perks[id]
end

--Returns true if this class participates in worker defense. False otherwise.
--Unless the class specifies in their _description, it's true by default.
function CrafingJob:get_worker_defense_participation()
   return self._sv.worker_defense_participant
end

-- Call when it's time to level up in this class
function CrafingJob:level_up()
   job_helper.level_up(self._sv)
   self.__saved_variables:mark_changed()
end

-- If the crafter has a workshop, associate it with the given talisman
-- Usually called when the talisman is spawned because the crafter has died or been demoted
function CrafingJob:associate_entities_to_talisman(talisman_entity)
   self._sv._entity:get_component('stonehearth:crafter'):associate_talisman_with_workshop(talisman_entity)
end

-- Call when it's time to demote
function CrafingJob:demote()
   self:_remove_xp_listeners()

   self._sv._entity:get_component('stonehearth:crafter'):demote()
   self._sv._entity:remove_component("stonehearth:crafter")
   
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

function CrafingJob:set_fine_percentage(args)
   self._sv._entity:get_component('stonehearth:crafter'):set_fine_percentage(args.percent_chance)
end

function CrafingJob:unset_fine_percentage(args)
   self._sv._entity:get_component('stonehearth:crafter'):set_fine_percentage(0)
end

--- Private functions

function CrafingJob:_create_xp_listeners()
   self._on_craft_listener = radiant.events.listen(self._sv._entity, 'stonehearth:crafter:craft_item', self, self._on_craft)
end

function CrafingJob:_remove_xp_listeners()
   self._on_craft_listener:destroy()
   self._on_craft_listener = nil
end

--When we've crafted an item, we get XP according to the min-crafter level of the item
--If there is no level specified it's a default and should get that amount
--TODO: some items should give extra bonuses if they're really cool
function CrafingJob:_on_craft(args)
   local recipe_data = args.recipe_data
   local craftable_uri = args.product_uri
   local number_crafted = self:get_number_crafted(craftable_uri)
   
   local exp_multiplier = 1
   if number_crafted == 0 then
      -- Add a curiosity multiplier
      local attributes_component = self._sv._entity:get_component('stonehearth:attributes')
      local curiosity = attributes_component:get_attribute('curiosity')
      exp_multiplier = curiosity * constants.attribute_effects.CURIOSITY_NEW_CRAFTABLE_MULTIPLER 
      if exp_multiplier < 1 then
         exp_multiplier = 1
      end
   end

   number_crafted = number_crafted + 1
   self._sv.num_crafted[craftable_uri] = number_crafted

   local level_key = 'craft_level_0'
   if recipe_data.level_requirement then
      level_key = 'craft_level_' .. recipe_data.level_requirement
   end
   local exp = self._sv.xp_rewards[level_key]
   exp = radiant.math.round(exp * exp_multiplier)
   self._job_component:add_exp(exp)
end

--Return the number of craftable_uri this crafter has crafted in their lifetime
function CrafingJob:get_number_crafted(craftable_uri)
   local number_crafted = self._sv.num_crafted[craftable_uri]
   return number_crafted or 0
end

return CrafingJob