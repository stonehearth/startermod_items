
local BuildUndoManager = class()

local log = radiant.log.create_logger('build.undo')

function BuildUndoManager:__init()
   self._changed_objects = {}
   self._object_traces = {}
   self._entity_traces = {}
   self._undo_stack = {}
   self._save_states = {}
   self._unlinked_entities = {}
   self._added_entities = {}
   self._tracer = _radiant.sim.create_tracer('undo manager')
   self._tracer_category = self._tracer.category
   
   self._tracer:stop()
end

function BuildUndoManager:begin_transaction(desc)
   log:detail('begin_transaction for "%s" (stack size:%d)', desc, #self._undo_stack)

   assert(not self._in_transaction)
   assert(not next(self._changed_objects))
   assert(not next(self._added_entities))
   assert(not next(self._unlinked_entities))

   self._in_transaction = true
   self._tracer:start()
end

function BuildUndoManager:end_transaction(desc)
   log:detail('end_transaction for "%s"', desc)

   self._tracer:flush()
   self._tracer:stop()

   -- compute the stack offset of the entry we're about to push onto
   -- the stack
   local stack_top = #self._undo_stack + 1
   
   for id, obj in pairs(self._changed_objects) do
      local entries = self._save_states[id]
      if not entries then
         entries = {}
         self._save_states[id] = entries
      end
      log:detail("saving object %d (%s)", id, tostring(obj))
      local entry = {
         id = id,
         obj = obj,
         stack_offset = stack_top,            
         save_state = _radiant.sim.save_object(id),
      }
      table.insert(entries, entry)
   end

   table.insert(self._undo_stack, {
         changed_objects = self._changed_objects,
         added_entities = self._added_entities,
         unlinked_entities = self._unlinked_entities,
      })
        
   self._in_transaction = false
   self._changed_objects = {}
   self._added_entities = {}
   self._unlinked_entities = {}
end

function BuildUndoManager:undo()
   local stack_top = #self._undo_stack
   log:detail('undo (stack size: %d)', stack_top)

   local datastores = {}  
   self._ignore_traces = true

   if stack_top > 0 then
      local entry = self._undo_stack[stack_top]
      table.remove(self._undo_stack, stack_top)
      stack_top = stack_top - 1

      -- load the state of the previous frame, if one exists.
      if stack_top > 0 then
         for id, _ in pairs(entry.changed_objects) do
            local entries = self._save_states[id]
            assert(entries)
            
            local o
            for i=#entries,1,-1 do
               if entries[i].stack_offset <= stack_top then
                  o = entries[i]
                  break
               end
            end
            if o then
               assert(o.stack_offset <= stack_top)
               _radiant.sim.load_object(o.save_state)
               log:detail("loading saved object %d", o.id)

               -- if the object we just loaded is a DataStore, hold onto it so
               -- we can restore the pointers to any controllers it references
               -- later
               if radiant.util.is_datastore(o.obj) then
                  table.insert(datastores, o.obj)
                  o.obj:mark_changed()
               end
            end
         end
      end

      -- loop through all the datastores we loaded and ask them to restore
      -- their internal references to other controllers.
      for _, datastore in ipairs(datastores) do
         datastore:restore()
      end
      
      -- put all the entities we unlink back where the belong
      for _, o in pairs(entry.unlinked_entities) do
         -- only really care if it was on the root.  if it was in the
         -- building structure, the load would have taken care of it.
         radiant.entities.add_child(o.parent, o.entity)
      end

      -- remove all the entities created by the last operation
   for _, o in pairs(entry.added_entities) do         
         radiant.entities.destroy_entity(o.entity)
      end
   end
end

function BuildUndoManager:unlink_entity(entity)
   if not self._in_transaction then
      log:error('unlinking entity outside of transaction!  clearing undo stack.')
      self:clear()
      return
   end

   log:detail('unlinking %s', entity)
   local entry = {
      entity = entity
   }

   -- remove it from its parent, leaving it hanging in space
   local mob = entity:get_component('mob')
   if mob then
      local parent = mob:get_parent()
      if parent then
         parent:get_component('entity_container')
                     :remove_child(entity:get_id())
         entry.parent = parent
      end
   end

   -- remove all dependencies
   local cp = entity:get_component('stonehearth:construction_progress')
   if cp then
      cp:unlink()
      local fabricator_entity = cp:get_fabricator_entity()
      if fabricator_entity and fabricator_entity ~= entity then
         self:unlink_entity(fabricator_entity)
      end
   end

   local fab_comp = entity:get_component('stonehearth:fabricator')
   if fab_comp then
      self:unlink_entity(fab_comp:get_project())
   end
   
   table.insert(self._unlinked_entities, entry)
end

function BuildUndoManager:_mark_changed(obj)
   local id = obj:get_id()
   -- if the object is a datastore, the id we want to save is actually
   -- the contains (theoretically opaque), DataObject contained inside it!
   if radiant.util.is_datastore(obj) then    
      id = obj:get_data_object_id()
   end
   self._changed_objects[id] = obj
end

function BuildUndoManager:trace_building(entity)
   log:detail('tracing new building %s', entity)

   assert(self._in_transaction)
   self:_trace_entity(entity)
end

function BuildUndoManager:_trace_entity(entity)
   local eid = entity:get_id()

   if self._entity_traces[eid] then
      log:detail('ignoring trace request for %s (already traced)', entity)
      return
   end
   
   local traces = {}
   self._entity_traces[eid] = traces


   local function trace_component(component, name)
      return component:trace('building undo.tc', self._tracer_category)
                     :on_changed(function()
                           if entity:is_valid() then
                              log:detail('component %s for entity %s changed', name, entity)
                              self:_mark_changed(component)
                           end
                        end)
                     :push_object_state()
   end
   
   local function trace_lua_component(component, name)
      return component.__saved_variables:trace('building undo.tlc', self._tracer_category)
                     :on_changed(function()
                           if entity:is_valid() then
                              log:detail('component %s for entity %s changed', name, entity)
                              self:_mark_changed(component.__saved_variables)
                           end
                        end)
                     :push_object_state()   
   end
   
   local function trace_region(component, name)
      return component:trace_region('building undo.tr', self._tracer_category)
                     :on_changed(function(r)
                           if entity:is_valid() then
                              local region = component:get_region()
                              if region then
                                 log:detail('%s region for entity %s changed (component:%d bounds:%s)', name, entity, region:get_id(), r:get_bounds())
                                 self:_mark_changed(region)
                              end
                           end
                        end)
                     :push_object_state()
   end
   
   local function trace_entity_container(component)
      return component:trace_children('building undo.tec', self._tracer_category)
                     :on_added(function(id, e)
                           log:detail('child %s added to entity %s', e, entity)
                           self:_trace_entity(e)
                        end)
                     :on_removed(function(id)
                           log:detail('child %d added to entity %s', id, entity)
                           self:_untrace_entity(id)
                        end)
                     :push_object_state()
   end
   
   traces.components = entity:trace_components('building undo.tc2', self._tracer_category)
                              :on_added(function (name)
                                    if entity:is_valid() then
                                       local component = entity:get_component(name)
                                       log:detail('tracing component %s for entity %s', name, entity)

                                       if not traces[name] then
                                          traces[name] = trace_component(component, name)
                                          if name == 'entity_container' then
                                             traces.entity_container_children = trace_entity_container(component)
                                          elseif name == 'destination' then
                                             traces.destination_region = trace_region(component, 'destination')
                                          elseif name == 'region_collision_shape' then
                                             traces.region_collision_shape_region = trace_region(component, 'region collision shape')
                                          end
                                       end
                                    end
                                 end)
                              :on_removed(function (name)
                                    log:detail('component %s for entity %s removed', name, entity)
                                    if traces[name] then
                                       traces[name]:destroy()
                                       traces[name] = nil
                                    end
                                 end)
                              :push_object_state()

   traces.lua_components = entity:trace_lua_components('building undo.tlc2', self._tracer_category)
                              :on_added(function (name)
                                    if entity:is_valid() then
                                       local component = entity:get_component(name)
                                       log:detail('tracing lua component %s for entity %s', name, entity)

                                       if not traces[name] then
                                          traces[name] = trace_lua_component(component, name)
                                       end
                                    end
                                 end)
                              :on_removed(function (name)
                                    log:detail('component %s for entity %s removed', name, entity)
                                    if traces[name] then
                                       traces[name]:destroy()
                                       traces[name] = nil
                                    end
                                 end)
                              :push_object_state()

   self._added_entities[eid] = {
      uri = entity:get_uri(),
      entity = entity,
   } 
end

function BuildUndoManager:clear()
   assert(not self._in_transaction)

   for eid, _ in pairs(self._entity_traces) do
      self:_untrace_entity(eid)
   end
   self:_clear_undo_stack()
   self._changed_objects = {}
   self._object_traces = {}
   self._entity_traces = {}
   self._save_states = {}
   self._unlinked_entities = {}
end

-- clear the undo stack.  each step in the undo stack includes a list of entities
-- which were removed from the world.  go through and delete all those entities if
-- they're still removed, since there will be no way to recover them when the undo
-- stack is nuked
--
function BuildUndoManager:_clear_undo_stack()
   for _, stack_entry in pairs(self._undo_stack) do
      for _, unlink_entry in pairs(stack_entry.unlinked_entities) do
         local entity = unlink_entry.entity
         if entity:is_valid() then
            -- we know the entity is unreachable if get_world_location returns
            -- nil
            if not radiant.entities.get_world_location(entity) then
               log:detail('destroying unlinked entity %s in clear()', entity)
               radiant.entities.destroy_entity(entity)
            end
         end
      end
   end
   self._undo_stack = {}
end

function BuildUndoManager:_untrace_entity(id)  
   local data = self._entity_traces[id]
   if data then
      for name, trace in pairs(data) do
         trace:destroy()
      end
      self._entity_traces[id] = nil
   end
end

return BuildUndoManager
