local Point3 = _radiant.csg.Point3
local RunToLocation = class()

RunToLocation.name = 'run to location'
RunToLocation.does = 'stonehearth:goto_location'
RunToLocation.args = {
   location = Point3,
   reason = 'string',
   stop_when_adjacent = {
      type = 'boolean',   -- whether to stop adjacent to destination
      default = false,
   },
}
RunToLocation.version = 2
RunToLocation.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RunToLocation)
         :execute('stonehearth:create_entity', {
            location = ai.ARGS.location,
            options = {
               debug_text = ai.ARGS.reason,
               add_item_destination = ai.NOT(ai.ARGS.stop_when_adjacent),
            }
         })
         :execute('stonehearth:goto_entity', {
            entity = ai.PREV.entity,
         })
