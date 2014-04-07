local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local RunTowardLocation = class()

RunTowardLocation.name = 'run toward location'
RunTowardLocation.does = 'stonehearth:go_toward_location'
RunTowardLocation.args = {
   location = Point3,
}
RunTowardLocation.version = 2
RunTowardLocation.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RunTowardLocation)
         :execute('stonehearth:create_proxy_entity', {
            location = ai.ARGS.location,
         })
         :execute('stonehearth:go_toward_entity', {
            entity = ai.PREV.entity
         })
