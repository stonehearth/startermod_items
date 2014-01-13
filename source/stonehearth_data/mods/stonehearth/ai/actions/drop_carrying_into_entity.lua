local Entity = _radiant.om.Entity
local DropCarryingIntoEntity = class()

DropCarryingIntoEntity.name = 'drop carrying'
DropCarryingIntoEntity.does = 'stonehearth:drop_carrying_into_entity'
DropCarryingIntoEntity.args = {
   entity  = Entity      -- what to drop it into
}
DropCarryingIntoEntity.version = 2
DropCarryingIntoEntity.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(DropCarryingIntoEntity)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.entity  })
         :execute('stonehearth:drop_carrying_into_entity_adjacent', { entity = ai.ARGS.entity })
