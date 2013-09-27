--[[
   Crafter.lua implements all functionality associated with a
   crafter citizen. 
]]

local Crafter = class()

function Crafter:__init(entity, data_binding)
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
function Crafter:extend(json)
   self:set_info(json)
end

function Crafter:set_info(info)
   self._info = info

   if info and info.recipe_list then
      self._recipe_list = radiant.resources.load_json(info.recipe_list)
      
      -- xxx: for now, just grab them all. =)
      self._data.craftable_recipes = self._recipe_list.craftable_recipes
      self._data_binding:mark_changed()
   end
end

--[[
   Tell the crafter to perform his work_effect (the animtion he does when he's crafting)
   ai: the ai required for the entity to perform anything
]]
function Crafter:perform_work_effect(ai)
   ai:execute('stonehearth.run_effect', self._info.work_effect)
end


function Crafter:set_workshop(workshop_component)
   self._workshop = workshop_component
end

function Crafter:get_workshop()
   return self._workshop
end

--[[
   Returns the object that the crafter displays while
   he or she is working
]]
function Crafter:get_intermediate_item()
   return self._info.intermediate_item
end

return Crafter