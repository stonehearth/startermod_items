local Entity = _radiant.om.Entity
local ClearItemAdjacent = class()

ClearItemAdjacent.name = 'clear item adjacent'
ClearItemAdjacent.does = 'stonehearth:clear_item_adjacent'
ClearItemAdjacent.args = {
   item = Entity
}
ClearItemAdjacent.version = 2
ClearItemAdjacent.priority = 1

function ClearItemAdjacent:run(ai, entity, args)
   local item = args.item
   radiant.check.is_entity(item)
   radiant.entities.turn_to_face(entity, item)
   ai:execute('stonehearth:run_effect', {effect = 'work'})
   radiant.entities.destroy_entity(item)
end

return ClearItemAdjacent