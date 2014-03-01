local PostureComponent = class()


function PostureComponent:__create(entity, json)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._set_postures = {}
   self._log = radiant.log.create_logger('posture')
   self._log:set_prefix(string.format('postures for %s', tostring(entity)))

   self._postures = {}
   self.__savestate = radiant.create_datastore({
      postures = self._set_postures
   })
end

function PostureComponent:get_posture()
   return self._set_postures[#self._set_postures]
end

function PostureComponent:set_posture(posture_name)
   self._log:debug('adding posture %s', posture_name)
   table.insert(self._set_postures, posture_name)
   radiant.events.trigger(self._entity, 'stonehearth:posture_changed')
   self.__savestate:mark_changed()
end

function PostureComponent:unset_posture(posture_name)
   for i, v in ipairs(self._set_postures) do 
      if v == posture_name then 
         self._log:debug('removing posture %s', posture_name)
         table.remove(self._set_postures, i)
         
         radiant.events.trigger(self._entity, 'stonehearth:posture_changed')
         self.__savestate:mark_changed()
         return
      end
   end
   self._log:debug('posture %s is not in table.  ignoring remove request.', posture_name)
end

return PostureComponent
