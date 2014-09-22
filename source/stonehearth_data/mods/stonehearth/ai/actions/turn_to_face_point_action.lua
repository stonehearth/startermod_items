local Point3 = _radiant.csg.Point3
local TurnToFaceEntity = class()

TurnToFaceEntity.name = 'turn to face point'
TurnToFaceEntity.does = 'stonehearth:turn_to_face_point'
TurnToFaceEntity.args = {
   point = Point3
}
TurnToFaceEntity.version = 2
TurnToFaceEntity.priority = 1

function TurnToFaceEntity:run(ai, entity, args)
   radiant.entities.turn_to_face(entity, args.point)
end

return TurnToFaceEntity
