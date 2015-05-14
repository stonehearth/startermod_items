
local ObserversComponent = class()

function ObserversComponent:initialize(entity, json)
   self._entity = entity

   self._sv = self.__saved_variables:get_data()
   self.__saved_variables:set_controller(self)

   self._log = radiant.log.create_logger('observers')
                          :set_entity(self._entity)

   if not self._sv._initialized then
      -- wait until the entity is completely initialized before piling all our
      -- observers and actions
      radiant.events.listen_once(entity, 'radiant:entity:post_create', function()
            self:_initialize(json)
         end)
   else
      -- we're loading so instead listen on game loaded
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            self:_initialize(json)
         end)
   end
end

function ObserversComponent:destroy()
   self:_clear_ref_counts()

   for uri, observer in pairs(self._sv._observers) do
      self:remove_observer(uri)
   end
end

function ObserversComponent:add_observer(uri)
   local ref_count = self:_add_ref(uri)
   if ref_count > 1 then
      return
   end

   assert(not self._sv._observers[uri])
   self._sv._observers[uri] = radiant.create_controller(uri, self._entity)

   self.__saved_variables:mark_changed()
end

function ObserversComponent:remove_observer(uri)
   local ref_count = self:_dec_ref(uri)
   if ref_count > 0 then
      return
   end

   local observer = self._sv._observers[uri]

   if observer then
      observer:destroy()
      self._sv._observers[uri] = nil
      self.__saved_variables:mark_changed()
   end
end

function ObserversComponent:_initialize(json)
   if not self._sv._initialized then
      self._sv._observers = {}
      self._sv._observer_ref_counts = {}

      for _, uri in ipairs(json.observers or {}) do
         self:add_observer(uri)
      end
   else
      -- observers will be rehydradated by controller infrastructure
   end
   self._sv._initialized = true
   self.__saved_variables:mark_changed()
end

function ObserversComponent:_add_ref(uri)
   local ref_count = self._sv._observer_ref_counts[uri] or 0
   ref_count = ref_count + 1
   self._sv._observer_ref_counts[uri] = ref_count
   self.__saved_variables:mark_changed()
   return ref_count
end

function ObserversComponent:_dec_ref(uri)
   local ref_count = self._sv._observer_ref_counts[uri] or 0
   ref_count = ref_count - 1

   if ref_count <= 0 then
      ref_count = 0
      self._sv._observer_ref_counts[uri] = nil
   else
      self._sv._observer_ref_counts[uri] = ref_count
   end

   self.__saved_variables:mark_changed()
   return ref_count
end

function ObserversComponent:_clear_ref_counts()
   for uri, count in pairs(self._sv._observer_ref_counts) do
      self._sv._observer_ref_counts[uri] = nil
   end
end

return ObserversComponent
