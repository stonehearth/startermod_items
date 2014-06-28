local Entity = _radiant.om.Entity
local GotoItemMadeOf = class()

GotoItemMadeOf.name = 'goto item made of'
GotoItemMadeOf.does = 'stonehearth:goto_item_made_of'
GotoItemMadeOf.args = {
   material = 'string',       -- the material tags we need
}
GotoItemMadeOf.think_output = {
   item = Entity,      -- the material tags we need
}
GotoItemMadeOf.version = 2
GotoItemMadeOf.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GotoItemMadeOf)
         :execute('stonehearth:material_to_filter_fn', { material = ai.ARGS.material })
         :execute('stonehearth:goto_entity_type', {
            filter_key = ai.PREV.filter_key,
            filter_fn = ai.PREV.filter_fn,
            description = ai.ARGS.material,
         })
         :set_think_output({ item = ai.PREV.destination_entity })
         
