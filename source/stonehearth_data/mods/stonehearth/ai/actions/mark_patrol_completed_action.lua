local MarkPatrolComplete = class()
MarkPatrolComplete.name = 'mark patrol compelte'
MarkPatrolComplete.does = 'stonehearth:mark_patrol_completed'
MarkPatrolComplete.args = {
   patrollable_object = 'table'
}
MarkPatrolComplete.version = 2
MarkPatrolComplete.priority = 1

function MarkPatrolComplete:run(ai, entity, args)
   stonehearth.town_defense:mark_patrol_completed(entity, args.patrollable_object)
end

return MarkPatrolComplete
