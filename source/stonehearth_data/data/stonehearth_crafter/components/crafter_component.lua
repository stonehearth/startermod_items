--[[
   Crafter.lua implements all functionality associated with a
   generic crafter. For example, a reference to the workshop,
   inspiration levels, etc. Individual crafter types (like carpenter)
   can inject their specific kind of crafter attributes through
   the extend mechanism on their classinfo.json documents.
]]

local Crafter = class()

function Crafter:__init(entity)
   self._entity = entity  -- the entity this component is attached to
   self._workshop = {}     -- the component for the crafter's workplace
end

--[[
   Takes extra arguments from class_info.json and
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

--[[
   Tell the crafter to perform his work_effect (the animtion he does when he's crafting)
   ai: the ai required for the entity to perform anything
]]
function Crafter:perform_work_effect(ai)
   ---[[
   --TODO: replace with ai:execute('stonehearth.activities.run_effect,''saw')
   --It does not seem to be working right now, but this does:
   self._curr_effect = radiant.effects.run_effect(self._entity, self._work_effect)
   ai:wait_until(function()
      return self._curr_effect:finished()
   end)
   self._curr_effect = nil
   --]]
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
   return self._intermediate_item
end

return Crafter