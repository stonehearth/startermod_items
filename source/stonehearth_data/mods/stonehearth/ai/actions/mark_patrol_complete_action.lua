local MarkPatrolComplete = class()
MarkPatrolComplete.name = 'mark patrol compelte'
MarkPatrolComplete.does = 'stonehearth:mark_patrol_complete'
MarkPatrolComplete.args = {}
MarkPatrolComplete.version = 2
MarkPatrolComplete.priority = 1

function MarkPatrolComplete:run(ai, entity, args)
   stonehearth.town_defense:mark_patrol_complete(entity)
end

return MarkPatrolComplete
