local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local CreateEntity = class()

--[[
   Action to create an entity at a location. 
   For use in compound actions
   If an entity type is not specified, then we create an empty, proxy entity
   If a location is specified, we put the entity at the location 
]]

CreateEntity.name = 'create designated'
CreateEntity.does = 'stonehearth:create_entity'
CreateEntity.args = {
   location = {
      type = Point3,     -- location to stick it
      default = stonehearth.ai.NIL
   }, 
   entity_type = {
      type = 'string', 
      default = stonehearth.ai.NIL
   }
}
CreateEntity.version = 2
CreateEntity.priority = 1

function CreateEntity:run(ai, entity, args)
   if self._new_entity then
      radiant.entities.destroy_entity(self._new_entity)
      self._new_entity = nil
   end

   if args.entity_type then
      self._new_entity = radiant.entities.create_entity(args.entity_type)
   else 
      self._new_entity = radiant.entities.create_entity()
   end
   if args.location then
      radiant.terrain.place_entity(self._new_entity, args.location)
   end
end


return CreateEntity
