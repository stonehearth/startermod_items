local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local SitOnChairAdjacent = class()
SitOnChairAdjacent.name = 'sit on chair adjacent'
SitOnChairAdjacent.does = 'stonehearth:sit_on_chair_adjacent'
SitOnChairAdjacent.args = {
   chair = Entity
}
SitOnChairAdjacent.version = 2
SitOnChairAdjacent.priority = 1

function SitOnChairAdjacent:run(ai, entity, args)
   local mob = entity:get_component('mob')
   local chair = args.chair

   self._saved_location = mob:get_world_grid_location()
   self._saved_facing = mob:get_facing()
   self._saved_collision_type = mob:get_mob_collision_type()

   -- change to collision type so we can place it inside the collision region
   mob:set_mob_collision_type(_radiant.om.Mob.NONE)
   radiant.entities.add_child(chair, entity, Point3.zero, true)
   radiant.entities.set_posture(entity, 'stonehearth:sitting_on_chair')
end

function SitOnChairAdjacent:stop(ai, entity, args)
   if self._saved_facing and self._saved_location and self._saved_collision_type then
      local root_entity = radiant.entities.get_root_entity()
      local mob = entity:get_component('mob')
      local egress_facing = self._saved_facing + 180

      radiant.entities.add_child(root_entity, entity, self._saved_location)
      mob:turn_to(egress_facing)
      mob:set_mob_collision_type(self._saved_collision_type)
      radiant.entities.unset_posture(entity, 'stonehearth:sitting_on_chair')
   end

   self._saved_location = nil
   self._saved_facing = nil
   self._saved_collision_type = nil
end

return SitOnChairAdjacent
