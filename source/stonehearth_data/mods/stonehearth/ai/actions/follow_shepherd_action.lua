local FollowShepherd = class()

FollowShepherd.name = 'follow shepherd'
FollowShepherd.does = 'stonehearth:top'
FollowShepherd.args = {}
FollowShepherd.version = 2
FollowShepherd.priority = stonehearth.constants.priorities.top.AMBIENT_PET_BEHAVIOR

local ai = stonehearth.ai
return ai:create_compound_action(FollowShepherd)
   :execute('stonehearth:select_shepherd')
   :execute('stonehearth:follow_entity', {target = ai.PREV.shepherd})
