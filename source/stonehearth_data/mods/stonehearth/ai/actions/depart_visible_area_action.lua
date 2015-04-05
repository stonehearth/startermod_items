local DepartVisibleArea = class()

DepartVisibleArea.name = 'depart visible area'
DepartVisibleArea.does = 'stonehearth:depart_visible_area'
DepartVisibleArea.args = {}
DepartVisibleArea.version = 2
DepartVisibleArea.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(DepartVisibleArea)
   :execute('stonehearth:find_point_beyond_visible_region') --actually is explored region, not visible region
   :execute('stonehearth:goto_location', {location = ai.PREV.location, reason = 'time to leave the world'})
   :execute('stonehearth:run_effect', {effect = '/stonehearth/data/effects/fursplosion_effect/growing_wool_effect.json'})
   :execute('stonehearth:destroy_entity') --TODO: rename destroy self to kill self, make new destroy_self, update loot component and others that listen on destroy
