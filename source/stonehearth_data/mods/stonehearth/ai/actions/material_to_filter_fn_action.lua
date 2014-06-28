local Entity = _radiant.om.Entity
local MaterialToFilterFn = class()

MaterialToFilterFn.name = 'material to filter fn'
MaterialToFilterFn.does = 'stonehearth:material_to_filter_fn'
MaterialToFilterFn.args = {
   material = 'string',      -- the material tags we need
}
MaterialToFilterFn.think_output = {
   filter_key = 'string',
   filter_fn = 'function',   -- a function which checks those tags
}
MaterialToFilterFn.version = 2
MaterialToFilterFn.priority = 1

function MaterialToFilterFn:start_thinking(ai, entity, args)
   local filter_fn = function(item)  
      return radiant.entities.is_material(item, args.material)
   end
   ai:set_think_output({
         filter_key = string.format('material: %s', args.material),
         filter_fn = filter_fn,
      })
end

return MaterialToFilterFn
