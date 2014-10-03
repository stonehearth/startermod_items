local CarpenterClass = class()

function CarpenterClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false
   self._sv.is_max_level = false
end

function CarpenterClass:restore()
end

function CarpenterClass:promote(json, talisman_entity)
   self._sv.is_current_class = true
   self._sv.job_name = json.name

   if json.level_data then
      self._sv.level_data = json.level_data
   end

   local crafter_component = self._sv._entity:add_component("stonehearth:crafter", json.crafter) 
   if talisman_entity then
      crafter_component:setup_with_existing_workshop(talisman_entity)
   end

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

-- If the carpenter has a workshop, associate it with the given talisman
-- Usually called when the talisman is spawned because the carpenter has died or been demoted
function CarpenterClass:associate_entities_to_talisman(talisman_entity)
   self._sv._entity:get_component('stonehearth:crafter'):associate_talisman_with_workshop(talisman_entity)
end

function CarpenterClass:demote()
   self._sv._entity:get_component('stonehearth:crafter'):demote()

   self._sv._entity:remove_component("stonehearth:crafter")
   
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

return CarpenterClass