local TaskGroup = class()

function TaskGroup:__init(scheduler)
   self._scheduler = scheduler

   -- a table of all the entities which belong to the task group.   only
   -- entities in this group may run these tasks.  entities can be in
   -- any number of task groups
   self._entities = {}        

   -- a table of all the tasks in this group.  a task must be in exactly
   -- one task group.
   self._tasks = {}
end

function TaskGroup:add_entity(entity)
   local id = entity:get_id()
   self._entities[id] = entity
end

function TaskGroup:remove_entity(id)
   self._entities[id] = nil
end

function TaskGroup:_recommend_entity_for_task(task)
end

function TaskGroup:create_task(name, args)
   assert(type(name) == 'string')
   assert(args == nil or type(args) == 'table')

   local activity = {
      name = name,
      args = args or {}
   }
   
   local task = Task(self, activity)
   self._tasks[task] = true;

   return task
end

return TaskGroup
