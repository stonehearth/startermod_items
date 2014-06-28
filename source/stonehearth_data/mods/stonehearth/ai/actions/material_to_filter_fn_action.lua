local Entity = _radiant.om.Entity
local MaterialToFilterFn = class()

MaterialToFilterFn.name = 'material to filter fn'
MaterialToFilterFn.does = 'stonehearth:material_to_filter_fn'
MaterialToFilterFn.args = {
   material = 'string',      -- the material tags we need
}
MaterialToFilterFn.think_output = {
   filter_fn = 'function',   -- a function which checks those tags
}
MaterialToFilterFn.version = 2
MaterialToFilterFn.priority = 1

ALL_FILTER_FNS = {}

function MaterialToFilterFn:start_thinking(ai, entity, args)
   -- make sure we return the exact same filter function for all
   -- materials so we can share the same pathfinders.  returning an
   -- equivalent implementation is not sufficient!  it must be
   -- the same function (see the 'stonehearth:pathfinder' component)
   
   local filter_fn = ALL_FILTER_FNS[args.key]
   if not filter_fn then
      local material = args.material
      filter_fn = function(item)
         return radiant.entities.is_material(item, args.material)
      end
      ALL_FILTER_FNS[material] = filter_fn
   end

   ai:set_think_output({
         filter_fn = filter_fn,
      })
end

return MaterialToFilterFn
