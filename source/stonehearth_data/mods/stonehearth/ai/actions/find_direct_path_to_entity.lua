local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local FindDirectPathToEntity = class()

FindDirectPathToEntity.name = 'find direct path to entity'
FindDirectPathToEntity.does = 'stonehearth:find_direct_path_to_entity'
FindDirectPathToEntity.args = {
   destination = Entity,   -- entity to find a path to
   allow_incomplete_path = {
      type = 'boolean',
      default = false,
   },
   reversible_path = {
      type = 'boolean',
      default = false,
   }
}
FindDirectPathToEntity.think_output = {
   path = Path,   -- the path to destination, from the current Entity
}
FindDirectPathToEntity.version = 2
FindDirectPathToEntity.priority = 1

function FindDirectPathToEntity:start_thinking(ai, entity, args)
   local destination = args.destination
   
   if not destination or not destination:is_valid() then
      ai:get_log():debug('invalid entity reference.  ignorning')
      return
   end

   local direct_path_finder = _radiant.sim.create_direct_path_finder(entity, destination)
                                 :set_allow_incomplete_path(args.allow_incomplete_path)
                                 :set_reversible_path(args.reversible_path)

   local path = direct_path_finder:get_path()
   if path then
      ai:set_think_output({ path = path })
      return
   end
   
   if args.allow_incomplete_path then
      -- we said we'd take an incomplete path, but the direct path finder wouldn't
      -- cooperate.  something is horribly wrong (e.g. the target entity have been
      -- destroyed).
      errror('could not find partial direct path from %s to %s.', entity, destination)
   end
end

return FindDirectPathToEntity
