
local EntityTracker = class()

--- Creates a list of objects that satisfies a set of criteria
--  The list will update whenever a new entity is add/removed
--  and whenever any of the specified events are triggered
--  @param tracker_name - used to identify the trace
--  @param filter_fn    - returns true if an object is worthy, false otherwise
--  @param event_array  - events we should listen to. Arr must contain the source 
--                  module, the event name, and events must send 'entity' to
--                  be evaluated.
function EntityTracker:__init(tracker_name, filter_fn, event_array)
   local added_cb = function(id, entity)
      self:_on_entity_add(id, entity)
   end
   local removed_cb = function(id)
      self:_on_entity_remove(id)
   end
   self._entities = {}
   self._tracked_entities = {} -- used to avoid an O(n) removal for non workers  
   self._filter_fn = filter_fn

   self.__savestate = radiant.create_datastore({
         entities = self._entities,
      })

   self._promise = radiant.terrain.trace_world_entities(tracker_name, added_cb, removed_cb)

   --Call our evaluation function each time each of these events fires.
   local object_tracker_service = stonehearth.object_tracker
   for i, event in ipairs(event_array) do
      radiant.events.listen(object_tracker_service, event.event_name, self, self.on_entity_change)
   end

end

function EntityTracker:_on_entity_add(id, entity)
   if self._filter_fn(entity) then
      self:_add_entity(entity)
   end
end

function EntityTracker:_add_entity(entity)
   table.insert(self._entities, entity)
   self.__savestate:mark_changed()
   self._tracked_entities[entity:get_id()] = true
end

--- Fires when an entity has been changed
--  If in its current state it no longer fits our filter function,
--  we should remove it. If in its current state it now fits our 
--  filter function and wasn't already present, we should add it. 
function EntityTracker:on_entity_change(e)
   local entity = e.entity
   if self._filter_fn(entity) then
      if not self._tracked_entities[entity:get_id()] then
         self:_add_entity(entity)
      end
   else
      self:_on_entity_remove(entity:get_id())
   end
end

function EntityTracker:_on_entity_remove(id)
   if self._tracked_entities[id] then
      self._tracked_entities[id] = nil

      -- Handlebars can't handle (heh) associative arrays (GAH!)
      for i, entity in ipairs(self._entities) do
         if entity and entity:get_id() == id then
            table.remove(self._entities, i)
            break
         end
      end
      self.__savestate:mark_changed()
   end
end

return EntityTracker
