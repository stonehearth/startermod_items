local farmer_class = {}

function farmer_class.promote(entity)
   local town = stonehearth.town:get_town(entity)
   town:join_task_group(entity, 'farmers')
end

function farmer_class.restore(entity)
   local town = stonehearth.town:get_town(entity)
   town:join_task_group(entity, 'farmers')
end

function farmer_class.demote(entity)
   local town = stonehearth.town:get_town(entity)
   town:leave_task_group(entity, 'farmers')
end

return farmer_class
