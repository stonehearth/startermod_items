
local ObserversComponent = class()

function ObserversComponent:initialize(entity, json)
   self._entity = entity

   self._observer_instances = {}
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
   self._dead = true
   for uri, _ in pairs(self._observer_instances) do
      self:remove_observer(uri)
   end
end

function ObserversComponent:add_observer(uri)
   local ref_count = self:_add_ref(uri)
   if ref_count > 1 then
      return
   end

   local ctor = radiant.mods.load_script(uri)
   local observer = ctor(self._entity)
   
   if not self._sv._observer_datastores[uri] then
      self._sv._observer_datastores[uri] = radiant.create_datastore()
   end

   observer.__saved_variables = self._sv._observer_datastores[uri]
   observer:initialize(self._entity)

   self._observer_instances[uri] = observer
   self.__saved_variables:mark_changed()
end

function ObserversComponent:remove_observer(uri)
   local ref_count = self:_dec_ref(uri)
   if ref_count > 0 then
      return
   end

   local observer = self._observer_instances[uri]

   if observer then
      if observer.destroy then
         observer:destroy()
      end

      self._observer_instances[uri] = nil
      if self._sv._observer_datastores[uri] then
         self._sv._observer_datastores[uri]:destroy()
         self._sv._observer_datastores[uri] = nil
      end
      self.__saved_variables:mark_changed()
   end
end

function ObserversComponent:_initialize(json)
   if not self._sv._initialized then
      self._sv._observer_datastores = {}
      self._sv._observer_ref_counts = {}

      for _, uri in ipairs(json.observers or {}) do
         self:add_observer(uri)
      end
   else
      for uri, _ in pairs(self._sv._observer_datastores) do
         self:add_observer(uri)
      end
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

return ObserversComponent
