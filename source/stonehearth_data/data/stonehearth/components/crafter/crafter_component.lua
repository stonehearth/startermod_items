--[[
   CrafterComponent.lua implements all functionality associated with a
   generic CrafterComponent. For example, a reference to the workshop,
   inspiration levels, etc. Individual CrafterComponent types (like carpenter)
   can inject their specific kind of CrafterComponent attributes through
   the extend mechanism on their classinfo.json documents.
]]

local CrafterComponent = class()

function CrafterComponent:__init(entity, data_binding)
   self._entity = entity   -- the entity this component is attached to
   self._workshop = nil    -- the component for the CrafterComponent's workplace
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
   if info and info.work_effect then
      self._work_effect = info.work_effect              --the effect to play when the CrafterComponent is working
   end

   if info and info.intermediate_item then
      self._intermediate_item = info.intermediate_item  --the object to show while the CrafterComponent is working
   end

   if info and info.recipe_list  then
      self._recipe_list = radiant.resources.load_json(info.recipe_list)
      
      -- xxx: for now, just grab them all. =)
      self._data.craftable_recipes = self._recipe_list.craftable_recipes
      self._data_binding:mark_changed()
   end
end

--[[
   Tell the CrafterComponent to perform his work_effect (the animtion he does when he's crafting)
   ai: the ai required for the entity to perform anything
]]
function CrafterComponent:perform_work_effect(ai)
   ai:execute('stonehearth.run_effect', self._work_effect)
end


function CrafterComponent:set_workshop(workshop_component)
   self._workshop = workshop_component
end

function CrafterComponent:get_workshop()
   return self._workshop
end

--[[
   Returns the object that the CrafterComponent displays while
   he or she is working
]]
function CrafterComponent:get_intermediate_item()
   return self._intermediate_item
end

return CrafterComponent