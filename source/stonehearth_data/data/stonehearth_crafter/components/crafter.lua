--[[
   Crafter.lua implements all functionality associated with a 
   generic crafter. For example, a reference to the workshop,
   inspiration levels, etc. Individual crafter types (like carpenter)
   can inject their specific kind of crafter attributes through
   the extend mechanism on their classinfo.txt documents.
]]

local Crafter = class()

function Crafter:__init(entity)
   self._workshop = {}     -- the component for the crafter's workplace
end

--[[
   Takes extra arguments from class_info.txt and
   injects them into the class. 
]]
function Crafter:extend(json)
   if json and json.work_effect then 
      self._work_effect = json.work_effect              --the effect to play when the crafter is working
   end
   if json and json.intermediate_item then
      self._intermediate_item = json.intermediate_item  --the object to show while the crafter is working
   end
end

function Crafter:set_workshop(workshop_component)
   self._workshop = workshop_component
end

function Crafter:get_workshop()
   return self._workshop
end

function Crafter:get_work_effect()
   return self._work_effect
end

function Crafter:get_intermediate_item()
   return self._intermediate_item
end

return Crafter