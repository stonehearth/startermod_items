local PostureComponent = class()

function PostureComponent:__init()
   self._log = radiant.log.create_logger('posture')
   self._set_postures = {}
end

function PostureComponent:initialize(entity, json)
   self._entity = entity
   self.__saved_variables = radiant.create_datastore({
      set_postures = self._set_postures
   })
end

function PostureComponent:restore(entity, saved_variables)
   self.__saved_variables = saved_variables
   saved_variables:read_data(function (o) 
         self._set_postures = o.set_postures
      end)
end

function PostureComponent:get_posture()
   return self._set_postures[#self._set_postures]
end

function PostureComponent:set_posture(posture_name)
   self._log:debug('adding posture %s', posture_name)
   table.insert(self._set_postures, posture_name)
   radiant.events.trigger(self._entity, 'stonehearth:posture_changed')
   self.__saved_variables:mark_changed()
end

function PostureComponent:unset_posture(posture_name)
   for i, v in ipairs(self._set_postures) do 
      if v == posture_name then 
         self._log:debug('removing posture %s', posture_name)
         table.remove(self._set_postures, i)
         
         radiant.events.trigger(self._entity, 'stonehearth:posture_changed')
         self.__saved_variables:mark_changed()
         return
      end
   end
   self._log:debug('posture %s is not in table.  ignoring remove request.', posture_name)
end

return PostureComponent
