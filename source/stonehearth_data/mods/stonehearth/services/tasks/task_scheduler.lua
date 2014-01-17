local Task = require 'services.tasks.task'
local RunTaskActionFactory = require 'services.tasks.run_task_action_factory'
local TaskScheduler = class()

function TaskScheduler:__init(name)
   self._name = name
   self._log = radiant.log.create_logger('tasks', name)
   self._tasks = {}
   self._entities = {}
   self._priority = 3
   self._log:debug('creating new task scheduler')
end

function TaskScheduler:get_name()
   return self._name
end

function TaskScheduler:destroy()
   for id, task in pairs(self._tasks) do
      task:destroy()
   end
   assert(radiant.util.table_is_empty(self._tasks))
   for id, entry in pairs(self._entities) do
      assert(radiant.util.table_is_empty(entry.run_action_tasks))
      self:leave(entry.entity)
   end
end

function TaskScheduler:set_activity(name, args)
   self._activity = stonehearth.ai:create_activity(name, args)
   self._log:debug('setting activity to %s', stonehearth.ai:format_activity(self._activity))
   return self
end

function TaskScheduler:create_task(name, args)
   assert(type(name) == 'string')
   assert(args == nil or type(args) == 'table')
   
   local activity = {
      name = name,
      args = args or {}
   }
   return Task(self, activity)
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
         run_task_actions = {},
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
         for task, action in ipairs(entry.run_task_actions) do
            self:_remove_action_from_entity(task, entry)
         end
         self._entities[id] = nil
      end
   end
   return self
end

function TaskScheduler:_add_action_to_entity(task, entry)
   assert(type(entry) == 'table') -- the entry, not the entity...
   local action = RunTaskActionFactory(task, self._priority, self._activity)
   stonehearth.ai:add_custom_action(entry.entity, action)
   entry.run_task_actions[task] = action
end

function TaskScheduler:_remove_action_from_entity(task, entry)
   assert(type(entry) == 'table') -- the entry, not the entity... 
   local action = entry.run_task_actions[task]
   if action then
      stonehearth.ai:remove_custom_action(entry.entity, action)
      entry.run_task_actions[task] = nil
   end
end

return TaskScheduler
