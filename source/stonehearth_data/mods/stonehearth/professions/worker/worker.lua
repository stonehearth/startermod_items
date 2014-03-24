local Point3 = _radiant.csg.Point3

local singleton = {
   _schedulers = {}
}
local worker_class = {}

function worker_class.promote(entity)
   worker_class.restore(entity)
end

function worker_class.restore(entity)
   local town = stonehearth.town:get_town(entity)
   town:join_task_group(entity, 'workers')
end

function worker_class.demote(entity)
   local town = stonehearth.town:get_town(entity)
   town:leave_task_group(entity, 'workers')
end


return worker_class
