--[[
   This component stores data about the job that a settler has.
]]

local ProfessionComponent = class()

function ProfessionComponent:__init()
   self._info = {}
end

function ProfessionComponent:extend(json)
   self:set_info(json)
end

function ProfessionComponent:set_info(info)
   self._info = info
end

function ProfessionComponent:get_name()
   return self._info.name
end

function ProfessionComponent:get_profession_type()
   return self._info.profession_type
end

function ProfessionComponent:get_class_script()
   return self._info.class_script
end

return ProfessionComponent
