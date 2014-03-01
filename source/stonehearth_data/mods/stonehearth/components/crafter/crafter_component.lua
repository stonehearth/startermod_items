--[[
   Crafter.lua implements all functionality associated with a
   crafter citizen. 
]]

local CrafterComponent = class()

function CrafterComponent:__create(entity, json)
   self._work_effect = json.work_effect
   if json.recipe_list then
      self._recipe_list = radiant.resources.load_json(json.recipe_list)
      self._craftable_recipes = self._recipe_list.craftable_recipes
   else
      self._recipe_list = {}
      self._craftable_recipes = {}
   end
   
   self.__savestate = radiant.create_datastore({
      craftable_recipes = self._craftable_recipes
   })
end

function CrafterComponent:get_work_effect()
   return self._work_effect
end

function CrafterComponent:set_workshop(workshop_component)
   self._workshop = workshop_component
end

function CrafterComponent:get_workshop()
   return self._workshop
end

return CrafterComponent
