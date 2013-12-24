--[[
   This component stores data about the job that a settler has.
]]

local ProfessionComponent = class()

function ProfessionComponent:__init(entity)
   self._entity = entity
   self._info = {}
end

function ProfessionComponent:extend(json)
   self:set_info(json)
end

function ProfessionComponent:set_info(info)
   self._info = info
   if self._info and self._info.name then 
      self._entity:add_component('unit_info'):set_description(self._info.name)

      --Let people know that the promotion has (probably) happened.
      local object_tracker_service = require 'services.object_tracker.object_tracker_service'
      radiant.events.trigger(object_tracker_service, 'stonehearth:promote', {entity = self._entity})
   end
end

function ProfessionComponent:get_name()
   return self._info.name
end

function ProfessionComponent:get_profession_type()
   return self._info.profession_type
end

function ProfessionComponent:get_script()
   return self._info.script
end

return ProfessionComponent
