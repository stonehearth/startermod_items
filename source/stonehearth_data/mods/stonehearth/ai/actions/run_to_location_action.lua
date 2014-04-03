local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local RunToLocation = class()

RunToLocation.name = 'run to location'
RunToLocation.does = 'stonehearth:goto_location'
RunToLocation.args = {
   location = Point3,
   stop_when_adjacent = {
      type = 'boolean',   -- whether to stop adjacent to destination
      default = false,
   }
}
RunToLocation.version = 2
RunToLocation.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RunToLocation)
         :execute('stonehearth:create_proxy_entity', {
            location = ai.ARGS.location,
            use_default_adjacent_region = ai.ARGS.stop_when_adjacent,
         })
         :execute('stonehearth:goto_entity', {
            entity = ai.PREV.entity
         })
