local Entity = _radiant.om.Entity
local MaterialToFilterFn = class()

MaterialToFilterFn.name = 'material to filter fn'
MaterialToFilterFn.does = 'stonehearth:material_to_filter_fn'
MaterialToFilterFn.args = {
   material = 'string',      -- the material tags we need
   owner = {                 -- must also be owed by
      type = 'string',
      default = '',
   }
}
MaterialToFilterFn.think_output = {
   filter_fn = 'function',   -- a function which checks those tags
   description = 'string',    -- a description of the filter   
}
MaterialToFilterFn.version = 2
MaterialToFilterFn.priority = 1

ALL_FILTER_FNS = {}

function MaterialToFilterFn:start_thinking(ai, entity, args)
   -- make sure we return the exact same filter function for all
   -- materials so we can share the same pathfinders.  returning an
   -- equivalent implementation is not sufficient!  it must be
   -- the same function (see the 'stonehearth:pathfinder' component)
   
   local key = string.format('material:%s owner:%s', args.material, args.owner)
   local filter_fn = ALL_FILTER_FNS[key]
   if not filter_fn then
      local owner = args.owner
      local material = args.material
      filter_fn = function(item)
         if not radiant.entities.is_material(item, args.material) then
            -- not the right material?  bail.
            return false
         end
         if owner ~= '' and radiant.entities.get_player_id(item) ~= owner then
            -- not owned by the right person?  also bail!
            return false
         end
         return true
      end
      ALL_FILTER_FNS[key] = filter_fn
   end

   ai:set_think_output({
         filter_fn = filter_fn,
         description = string.format('material is "%s"', args.material),
      })
end

return MaterialToFilterFn
