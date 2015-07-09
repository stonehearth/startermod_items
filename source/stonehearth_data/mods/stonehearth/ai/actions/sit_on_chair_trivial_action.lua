local SitOnChairTrivial = class()

SitOnChairTrivial.name = 'sit on chair trivial'
SitOnChairTrivial.does = 'stonehearth:sit_on_chair'
SitOnChairTrivial.args = {}
SitOnChairTrivial.version = 2
SitOnChairTrivial.priority = 1

function SitOnChairTrivial:start_thinking(ai, entity, args)
   local parent = radiant.entities.get_parent(entity)
   if not parent then
      return
   end

   if radiant.entities.get_entity_data(parent, 'stonehearth:chair') then
      -- hey, we're already in a chair!
      self._chair = parent
      ai:set_think_output()
   end
end

function SitOnChairTrivial:stop(ai, entity, args)
   if self._chair:is_valid() then
      local root_entity = radiant.entities.get_root_entity()
      local chair_location = radiant.entities.get_world_grid_location(self._chair)

      -- temporary hack so we can push to steam
      entity:add_component('mob'):set_mob_collision_type(_radiant.om.Mob.HUMANOID)
      radiant.entities.add_child(root_entity, entity, chair_location)

      self._chair = nil
   end
end

return SitOnChairTrivial
