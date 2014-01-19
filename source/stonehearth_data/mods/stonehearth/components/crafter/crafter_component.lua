--[[
   Crafter.lua implements all functionality associated with a
   crafter citizen. 
]]

local CrafterComponent = class()

function CrafterComponent:__init(entity, data_binding)
   self._entity = entity   -- the entity this component is attached to
   self._data = {
      craftable_recipes = {}
   }
   self._data_binding = data_binding
   self._data_binding:update(self._data)
end

--[[
   Takes extra arguments from class_info.json and
   injects them into the class.
]]
function CrafterComponent:extend(json)
   self:set_info(json)
end

function CrafterComponent:set_info(info)
   self._info = info

   if info and info.recipe_list then
      self._recipe_list = radiant.resources.load_json(info.recipe_list)
      
      -- xxx: for now, just grab them all. =)
      self._data.craftable_recipes = self._recipe_list.craftable_recipes
      self._data_binding:mark_changed()
   end
end

function CrafterComponent:set_workshop(workshop_component)
   self._workshop = workshop_component
end

function CrafterComponent:get_workshop()
   return self._workshop
end

return CrafterComponent
