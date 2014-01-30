local PostureComponent = class()


function PostureComponent:__init(entity, data_object)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._set_postures = {}
   self._log = radiant.log.create_logger('posture')
   self._log:set_prefix(string.format('postures for %s', tostring(entity)))

   self._data_object = data_object
   --local data = data_object:get_data()
   --data.postures = self._set_postures
   self._data_object:mark_changed()
end

function PostureComponent:trace(reason)
   return self._data_object:trace_data(reason)
end

function PostureComponent:get_posture()
   return self._set_postures[#self._set_postures]
end

function PostureComponent:set_posture(posture_name)
   self._log:debug('adding posture %s', posture_name)
   table.insert(self._set_postures, posture_name)
   self._data_object:mark_changed()
end

function PostureComponent:unset_posture(posture_name)
   for i, v in ipairs(self._set_postures) do 
      if v == posture_name then 
         self._log:debug('removing posture %s', posture_name)
         table.remove(self._set_postures, i)
         self._data_object:mark_changed()
         return
      end
   end
   self._log:debug('posture %s is not in table.  ignoring remove request.', posture_name)
end

return PostureComponent
