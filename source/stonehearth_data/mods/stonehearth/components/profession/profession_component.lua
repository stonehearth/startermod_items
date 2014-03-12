--[[
   This component stores data about the job that a settler has.
]]

local ProfessionComponent = class()

function ProfessionComponent:initialize(entity, json)
   self._entity = entity
   self._info = json
   if self._info.name then 
      self._entity:add_component('unit_info'):set_description(self._info.name)

      --Let people know that the promotion has (probably) happened.
      -- xxx: is there a better way?  How about if the town listens to all 'stonehearth:profession_changed'
      -- messages from its citizens?  That sounds good!!
      radiant.events.trigger(stonehearth.object_tracker, 'stonehearth:promote', {entity = self._entity})

      -- so good!  keep this one, lose the top one.  too much "collusion" between components =)
      radiant.events.trigger(self._entity, 'stonehearth:profession_changed', { entity = entity })
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
