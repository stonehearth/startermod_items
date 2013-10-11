radiant.mods.load('stonehearth')

local PostureComponent = class()

function PostureComponent:__init(entity)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._set_postures = {}
end

function PostureComponent:get_posture()
   return self._set_postures[#self._set_postures]
end

function PostureComponent:set_posture(posture_name)
   table.insert(self._set_postures, posture_name)
end

function PostureComponent:unset_posture(posture_name)
   for i, v in ipairs(self._set_postures) do 
      if v == posture_name then 
         table.remove(self._set_postures, i)
         return
      end
   end
end

return PostureComponent
