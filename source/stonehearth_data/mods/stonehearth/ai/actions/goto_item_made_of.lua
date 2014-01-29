local Entity = _radiant.om.Entity
local GotoItemMadeOf = class()

GotoItemMadeOf.name = 'goto item made of'
GotoItemMadeOf.does = 'stonehearth:goto_item_made_of'
GotoItemMadeOf.args = {
   material = 'string',      -- the material tags we need
   reconsider_event_name = {
      type = 'string',
      default = '',
   }
}
GotoItemMadeOf.think_output = {
   item = Entity,      -- the material tags we need
}
GotoItemMadeOf.version = 2
GotoItemMadeOf.priority = 1

local is_material = function (material, item)
   return radiant.entities.is_material(item, material)
end

local ai = stonehearth.ai
return ai:create_compound_action(GotoItemMadeOf)
         :execute('stonehearth:material_to_filter_fn', { material = ai.ARGS.material })
         :execute('stonehearth:goto_entity_type', {
            filter_fn = ai.PREV.filter_fn,
            reconsider_event_name = ai.ARGS.reconsider_event_name,
         })
         :set_think_output({ item = ai.PREV.destination_entity })
         
