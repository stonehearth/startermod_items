
local EntityTracker = class()

function EntityTracker:__init(filter_fn)
   local added_cb = function(id, entity)
      self:_on_entity_add(id, entity)
   end
   local removed_cb = function(id)
      self:_on_entity_remove(id)
   end
   self._data_store = _radiant.sim.create_data_store()
   self._data = self._data_store:get_data()
   self._data.entities = {}
   self._tracked_entities = {} -- used to avoid an O(n) removal for non workers
   
   self._filter_fn = filter_fn
   self._promise = radiant.terrain.trace_world_entities('census of workers', added_cb, removed_cb)
end

function EntityTracker:get_data_store()
   return self._data_store
end

function EntityTracker:_on_entity_add(id, entity)
   if self._filter_fn(entity) then
      table.insert(self._data.entities, entity)
      self._data_store:mark_changed()
      self._tracked_entities[entity:get_id()] = true
   end
end

function EntityTracker:_on_entity_remove(id)
   if self._tracked_entities[id] then
      self._tracked_entities[id] = nil
      self._data_store:mark_changed()

      -- Handlebars can't handle (heh) associative arrays (GAH!)
      for i, entity in ipairs(self._data.entities) do
         if entity and entity:get_id() == id then
            table.remove(self._data.entities, i)
            break
         end
      end
   end
end

return EntityTracker
