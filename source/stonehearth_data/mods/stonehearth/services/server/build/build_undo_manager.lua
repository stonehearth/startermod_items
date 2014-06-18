
local BuildUndoManager = class()

local log = radiant.log.create_logger('build.undo')

function BuildUndoManager:__init()
   if true then return end -- oops!
   self._changed_objects = {}
   self._object_traces = {}
   self._entity_traces = {}
   self._undo_stack = {}
   self._save_states = {}
   self._tracer = _radiant.sim.create_tracer()
   self._tracer_category = self._tracer.category

   self._stack_offset = 0
end

function BuildUndoManager:begin_transaction(desc)
   if true then return end -- oops!
   log:detail('begin_transaction for "%s" (stack size:%d)', desc, #self._undo_stack)

   self._in_transaction = true
   self._changed_objects = {}
   self._added_entities = {}
   self._unlinked_entities = {}
   for i = #self._undo_stack, self._stack_offset+1, -1 do
      table.remove(self._undo_stack, i)
   end
end

function BuildUndoManager:end_transaction(desc)
   if true then return end -- oops!
   log:detail('end_transaction for "%s"', desc)

   self._tracer:flush()
   
   assert(self._stack_offset == #self._undo_stack)

   self._stack_offset = self._stack_offset + 1
   
   for id, obj in pairs(self._changed_objects) do
      local id = obj:get_id()
      local entries = self._save_states[obj:get_id()]
      if not entries then
         entries = {}
         self._save_states[id] = entries
      end
      table.insert(entries, {
            stack_offset = self._stack_offset,
            save_state = _radiant.sim.save_object(id),
         })
   end

   table.insert(self._undo_stack, {
         changed_objects = self._changed_objects,
         added_entities = self._added_entities,
         unlinked_entities = self._unlinked_entities,
      })
      
   assert(self._stack_offset == #self._undo_stack)
   
   self._in_transaction = false
end

function BuildUndoManager:undo()
   if true then return end -- oops!
   log:detail('undo (stack size: %d)', self._stack_offset)

   self._ignore_traces = true

   if self._stack_offset > 0 then
      local entry = self._undo_stack[self._stack_offset]
      table.remove(self._undo_stack, self._stack_offset)

      -- load the state of the previous frame, if one exists.
      local last_offset = self._stack_offset - 1
      if last_offset > 0 then
         for id, _ in pairs(entry.changed_objects) do
            local entries = self._save_states[id]
            assert(entries)
            
            local o
            for i=#entries,1,-1 do
               if entries[i].stack_offset <= last_offset then
                  o = entries[i]
                  break
               end
            end
            if o then
               assert(o.stack_offset <= last_offset)
               _radiant.sim.load_object(o.save_state)
            end
         end
      end
      
      -- put all the entities we unlink back where the belong
      for _, o in pairs(entry.unlinked_entities) do
         -- only really care if it was on the root.  if it was in the
         -- building structure, the load would have taken care of it.
         radiant.entities.add_child(radiant._root_entity, o.entity)
      end

      -- remove all the entities created by the last operation
      for _, o in pairs(entry.added_entities) do         
         radiant.entities.destroy_entity(o.entity)
      end

      self._stack_offset = self._stack_offset - 1
   end
end

function BuildUndoManager:unlink_entity(entity)
   log:detail('unlinking %s', entity)
   local entry = {
      entity = entity
   }

   -- remove it from its parent, leaving it hanging in space
   local mob = entity:get_component('mob')
   if mob then
      local parent = mob:get_parent()
      if parent then
         parent:get_component('entity_container'):remove_child(entity:get_id())
         entry.parent = parent
      end
   end

   -- remove all dependencies
   local cp = entity:get_component('stonehearth:construction_progress')
   if cp then
      cp:unlink()
      local fabricator_entity = cp:get_fabricator_entity()
      if fabricator_entity then
         self:unlink_entity(fabricator_entity)
      end
   end
   table.insert(self._unlinked_entities, entry)
end

function BuildUndoManager:mark_changed(obj)
   self._changed_objects[obj:get_id()] = obj
end

function BuildUndoManager:trace_building(entity)
   if true then return end -- oops
   log:detail('tracing new building %s', entity)

   assert(self._in_transaction)
   self:_trace_entity(entity)
end

function BuildUndoManager:_trace_entity(entity)
   local eid = entity:get_id()
   assert(not self._entity_traces[eid])

   if self._entity_traces[eid] then
      log:detail('ignoring trace request for %s (already traced)', entity)
   end
   local traces = {}
   self._entity_traces[eid] = traces

   traces.entity = entity:trace_components('building undo', self._tracer_category)
                              :on_added(function (name)
                                    local obj = entity:get_component(name)
                                    log:detail('tracing component %s for entity %s', name, entity)

                                    if not traces[name] then
                                       traces[name] = obj:trace('building undo', self._tracer_category)
                                                         :on_changed(function()
                                                               log:detail('component %s for entity %s changed', name, entity)
                                                               self:mark_changed(obj)
                                                            end)
                                                         :push_object_state()
                                       if name == 'entity_container' then
                                          traces.entity_container_children = 
                                                      obj:trace_children('building undo', self._tracer_category)
                                                            :on_added(function(id, e)
                                                                  log:detail('child %s added to entity %s', e, entity)
                                                                  self:_trace_entity(e)
                                                               end)
                                                            :on_removed(function(id)
                                                                  log:detail('child %d added to entity %s', id, entity)
                                                                  self:_untrace_entity(id)
                                                               end)
                                                            :push_object_state()
                                       elseif name == 'destination' then
                                          traces.destination_region = 
                                                      obj:trace_region('building undo', self._tracer_category)
                                                            :on_changed(function(r)
                                                                  log:detail('destination region for entity %s changed (obj:%d bounds:%s)', entity, obj:get_region():get_id(), r:get_bounds())
                                                                  self:mark_changed(obj:get_region())
                                                               end)
                                       elseif name == 'region_collision_shape' then
                                          traces.region_collision_shape_region = 
                                                      obj:trace_region('building undo', self._tracer_category)
                                                            :on_changed(function(r)
                                                                  log:detail('region collision shape region for entity %s changed (obj:%d bounds:%s)', entity, obj:get_region():get_id(), r:get_bounds())
                                                                  self:mark_changed(obj:get_region())
                                                               end)
                                       end
                                    end
                                 end)
                              :on_removed(function (name)
                                    log:detail('component %s for entity %s removed', entity, entity)
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