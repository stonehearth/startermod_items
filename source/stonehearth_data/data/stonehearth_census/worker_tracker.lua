
local WorkerTracker = class()

-- xxx: why can't we pass in something htat look liks this? { faction = 'civ', class = 'worker' }
function WorkerTracker:__init(data_store)
   local added_cb = function(id, entity)
      self:_on_entity_add(id, entity)
   end
   local removed_cb = function(id)
      self:_on_entity_remove(id)
   end

   self._promise = radiant.terrain.trace_world_entities('census of workers', added_cb, removed_cb)
   self._data = data_store:get_data()
   if not self._data.entities then
      self._data.entities = {}
   end

   self._data_store = data_store
   self._tracked_entities = {} -- used to avoid an O(n) removal for non workers
end

function WorkerTracker:_on_entity_add(id, entity) 
   local job_info = entity:get_component('stonehearth_classes:job_info')
   if job_info then
      if job_info:get_id() == 'worker' then
         table.insert(self._data.entities, entity)
         self._data_store:mark_changed()
         self._tracked_entities[entity:get_id()] = true
      end
   end
end

function WorkerTracker:_on_entity_remove(id)
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

return WorkerTracker
