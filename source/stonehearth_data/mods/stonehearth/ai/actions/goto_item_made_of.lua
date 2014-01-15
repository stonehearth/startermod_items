local Entity = _radiant.om.Entity
local GotoItemMadeOf = class()

GotoItemMadeOf.name = 'goto item made of'
GotoItemMadeOf.does = 'stonehearth:goto_item_made_of'
GotoItemMadeOf.args = {
   material = 'string',      -- the material tags we need
}
GotoItemMadeOf.think_output = {
   item = Entity,      -- the material tags we need
}
GotoItemMadeOf.version = 2
GotoItemMadeOf.priority = 1

function GotoItemMadeOf:transform_arguments(ai, entity, args)
   return {
      filter_fn = function(item)
            return radiant.entities.is_material(item, args.material)
         end
   }
end

local ai = stonehearth.ai
return ai:create_compound_action(GotoItemMadeOf)
         :execute('stonehearth:goto_entity_type', { filter_fn = ai.XFORMED_ARG.filter_fn })
         :set_think_output({ item = ai.PREV.destination_entity })
         
