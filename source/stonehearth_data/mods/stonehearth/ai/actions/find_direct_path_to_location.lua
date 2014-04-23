local Path = _radiant.sim.Path
local Point3 = _radiant.csg.Point3
local FindDirectPathToLocation = class()

FindDirectPathToLocation.name = 'find direct path to location'
FindDirectPathToLocation.does = 'stonehearth:find_direct_path_to_location'
FindDirectPathToLocation.args = {
   destination = Point3,
   allow_incomplete_path = {
      type = 'boolean',
      default = false,
   },
   reversible_path = {
      type = 'boolean',
      default = false,
   }
}
FindDirectPathToLocation.think_output = {
   path = Path,   -- the path to destination, from the current Entity
}
FindDirectPathToLocation.version = 2
FindDirectPathToLocation.priority = 1

function FindDirectPathToLocation:start_thinking(ai, entity, args)
   local destination = args.destination
   
   local direct_path_finder = _radiant.sim.create_direct_path_finder(entity)
                                 :set_start_location(ai.CURRENT.location)
                                 :set_end_location(destination)
                                 :set_allow_incomplete_path(args.allow_incomplete_path)
                                 :set_reversible_path(args.reversible_path)

   local path = direct_path_finder:get_path()
   if path then
      ai:set_think_output({ path = path })
      return
   end
   
   if args.allow_incomplete_path then
      -- we said we'd take an incomplete path, but the direct path finder wouldn't
      -- cooperate.  something is horribly wrong
      error(string.format('could not find partial direct path from %s to %s.', tostring(entity), tostring(destination)))
   end
end

return FindDirectPathToLocation
