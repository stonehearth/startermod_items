
local ObserversComponent = class()

function ObserversComponent:initialize(entity, json)
   self._entity = entity

   self._sv = self.__saved_variables:get_data()
   self.__saved_variables:set_controller(self)

   self._log = radiant.log.create_logger('observers')
                          :set_entity(self._entity)

   if not self._sv._initialized then
      if json.observers then
         self._log:error('%s: Observers are now added through the ai_packs in entity_data. See base_human.json for an example.', entity)
         assert(false)
      end

      self._sv._observers = {}
      self._sv._ref_counts = radiant.create_controller('stonehearth:lib:reference_counter')
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
      -- the self._sv._observers instances are rehydrated by the controller infrastructure
   end
end

function ObserversComponent:destroy()
   self._sv._ref_counts:clear()

   for uri, observer in pairs(self._sv._observers) do
      self:remove_observer(uri)
   end

   self._sv._ref_counts:destroy()
end

function ObserversComponent:add_observer(uri)
   local ref_count = self._sv._ref_counts:add_ref(uri)
   if ref_count > 1 then
      return
   end

   assert(not self._sv._observers[uri])
   self._sv._observers[uri] = radiant.create_controller(uri, self._entity)

   self.__saved_variables:mark_changed()
end

function ObserversComponent:remove_observer(uri)
   local ref_count = self._sv._ref_counts:dec_ref(uri)
   if ref_count > 0 then
      return
   end

   local observer = self._sv._observers[uri]

   if observer then
      self._sv._observers[uri] = nil
      observer:destroy()
      self.__saved_variables:mark_changed()
   end
end

return ObserversComponent
