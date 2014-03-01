--[[
   This component stores data about the job that a settler has.
]]

local ProfessionComponent = class()

function ProfessionComponent:__create(entity, json)
   self._entity = entity
   self._info = json
   if self._info.name then 
      self._entity:add_component('unit_info'):set_description(self._info.name)

      --Let people know that the promotion has (probably) happened.
      local object_tracker_service = stonehearth.object_tracker
      radiant.events.trigger(object_tracker_service, 'stonehearth:promote', {entity = self._entity})
   end
end

function ProfessionComponent:change_profession(json)
   self._info = json
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
