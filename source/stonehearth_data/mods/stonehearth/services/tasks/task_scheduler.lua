local Task = require 'services.tasks.task'
local CompoundTask = require 'services.tasks.compound_task'
local TaskScheduler = class()

function TaskScheduler:__init(name)
   self._name = name
   self._log = radiant.log.create_logger('tasks', name)
   self._tasks = {}
   self._compound_tasks = {}
   self._entities = {}
   self._priority = 3
   self._log:debug('creating new task scheduler')
end

function TaskScheduler:get_name()
   return self._name
end

function TaskScheduler:destroy()
   for compound_task, _ in pairs(self._compound_tasks) do
      compound_task:destroy()
   end
   self._compound_tasks = {}

   for id, task in pairs(self._tasks) do
      task:destroy()
   end
   assert(radiant.util.table_is_empty(self._tasks))
   for id, entry in pairs(self._entities) do
      assert(radiant.util.table_is_empty(entry.tasks))
      self:leave(entry.entity)
   end
end

function TaskScheduler:set_activity(name, args)
   self._activity = stonehearth.ai:create_activity(name, args)
   self._log:debug('setting activity to %s', stonehearth.ai:format_activity(self._activity))
   return self
end

function TaskScheduler:get_activity()
   return self._activity
end


function TaskScheduler:create_task(name, args, co)
   assert(type(name) == 'string')
   assert(args == nil or type(args) == 'table')

   local activity = {
      name = name,
      args = args or {}
   }
   
   return Task(self, activity)
end

function TaskScheduler:create_orchestrator(name, args, co)
   assert(type(name) == 'string')
   assert(args == nil or type(args) == 'table')

   local activity = {
      name = name,
      args = args or {}
   }
   
   local compound_task_ctor = radiant.mods.load_script(name)
   local ct = CompoundTask(self, compound_task_ctor, activity, co)
   self._compound_tasks[ct] = true
   return ct
end

function TaskScheduler:_commit_task(task)
   self._tasks[task:get_id()] = task
   for id, entry in pairs(self._entities) do
      self:_add_action_to_entity(task, entry)
   end
end

function TaskScheduler:_decommit_task(task)
   local id = task:get_id()
   if self._tasks[id] then
      self._tasks[id] = nil
      for id, entry in pairs(self._entities) do
         self:_remove_action_from_entity(task, entry)
      end
   end
end

function TaskScheduler:join(entity)
   --assert(self._activity, 'must call set_activity before adding entities')
   
   self._log:debug('adding %s to task scheduler', entity)
   local id = entity:get_id()
   if not self._entities[id] then      
      local entry = {
         entity = entity,
         tasks = {},
      }
      for id, task in pairs(self._tasks) do
         self:_add_action_to_entity(task, entry)
      end
      self._entities[id] = entry
   end
   return self
end

function TaskScheduler:leave(entity) 
   if entity and entity:is_valid() then
      self._log:debug('removing %s from task scheduler', entity)
      local id = entity:get_id()
      local entry = self._entities[id]
      if entry then
         for task, action in ipairs(entry.tasks) do
            self:_remove_action_from_entity(task, entry)
         end
         self._entities[id] = nil
      end
   end
   return self
end

function TaskScheduler:_add_action_to_entity(task, entry)
   assert(type(entry) == 'table') -- the entry, not the entity...
   local action_ctor = task:get_action_ctor()
   stonehearth.ai:add_custom_action(entry.entity, action_ctor)
   entry.tasks[task] = task
end

function TaskScheduler:_remove_action_from_entity(task, entry)
   assert(type(entry) == 'table') -- the entry, not the entity... 
   local task = entry.tasks[task]
   if task then
      local action_ctor = task:get_action_ctor()
      stonehearth.ai:remove_custom_action(entry.entity, action_ctor)
      entry.tasks[task] = nil
   end
end

return TaskScheduler
