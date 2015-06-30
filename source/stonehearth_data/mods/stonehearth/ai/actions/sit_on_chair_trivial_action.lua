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
      ai:set_think_output()
   end
end

return SitOnChairTrivial
