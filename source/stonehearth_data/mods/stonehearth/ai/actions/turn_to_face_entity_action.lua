local Entity = _radiant.om.Entity
local TurnToFaceEntity = class()

TurnToFaceEntity.name = 'turn to face an entity'
TurnToFaceEntity.does = 'stonehearth:turn_to_face_entity'
TurnToFaceEntity.args = {
   entity = Entity
}
TurnToFaceEntity.version = 2
TurnToFaceEntity.priority = 1

function TurnToFaceEntity:run(ai, entity, args)
   radiant.check.is_entity(args.entity)
   radiant.entities.turn_to_face(entity, args.entity)
end

return TurnToFaceEntity
