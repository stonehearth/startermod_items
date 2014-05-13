local PostureComponent = class()

function PostureComponent:__init()
end

function PostureComponent:initialize(entity, json)
   self._log = radiant.log.create_logger('posture')
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.set_postures then
      self._sv.set_postures = {}
      for index, posture in pairs(json) do
         self:set_posture(posture)
      end
   end
end

function PostureComponent:get_posture()
   return self._sv.set_postures[#self._sv.set_postures]
end

function PostureComponent:set_posture(posture_name)
   self._log:debug('%s adding posture %s', self._entity, posture_name)
   table.insert(self._sv.set_postures, posture_name)
   radiant.events.trigger_async(self._entity, 'stonehearth:posture_changed')
   self.__saved_variables:mark_changed()
end

function PostureComponent:unset_posture(posture_name)
   for i, v in ipairs(self._sv.set_postures) do 
      if v == posture_name then 
         self._log:debug('%s removing posture %s', self._entity, posture_name)
         table.remove(self._sv.set_postures, i)
         
         radiant.events.trigger_async(self._entity, 'stonehearth:posture_changed')
         self.__saved_variables:mark_changed()
         return
      end
   end
   self._log:debug('posture %s is not in table.  ignoring remove request.', posture_name)
end

return PostureComponent
