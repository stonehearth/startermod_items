--[[
   This component stores data about the profession associated with
   an object.
]]

local ProfessionInfo = class()

function ProfessionInfo:__init()
   self._profession = nil
   self._workshop = nil
end

function ProfessionInfo:extend(json)
   if json and json.profession then
      self._profession = json.profession
   end
end

function ProfessionInfo:get_profession()
   return self._profession
end

--[[Place to store profession-specific data
--]]

--[[
   The workshop component, if this profession is a crafter
]]
function ProfessionInfo:set_workshop(workshop)
   self._workshop = workshop
end

function ProfessionInfo:get_workshop()
   return self._workshop
end

return ProfessionInfo
