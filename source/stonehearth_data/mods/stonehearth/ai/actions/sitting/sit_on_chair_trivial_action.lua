local SitOnChairTrivial = class()

SitOnChairTrivial.name = 'sit on chair trivial'
SitOnChairTrivial.does = 'stonehearth:sit_on_chair'
SitOnChairTrivial.args = { }
SitOnChairTrivial.version = 2
SitOnChairTrivial.priority = 1

function SitOnChairTrivial:start_thinking(ai, entity, args)
   local parent = radiant.entities.get_parent(entity)
   if not parent then
      return
   end

   if radiant.entities.get_entity_data(parent, 'stonehearth:chair') then
      -- hey, we're already in a chair!
      local chair = parent
      local mount_component = chair:add_component('stonehearth:mount')
      assert(mount_component:get_user() == entity)

      ai:set_think_output({ chair = chair })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(SitOnChairTrivial)
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.chair })
         :execute('stonehearth:sit_on_current_chair', { chair = ai.BACK(2).chair })
