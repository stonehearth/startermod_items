local Entity = _radiant.om.Entity

local SitOnCurrentChair = class()

SitOnCurrentChair.name = 'sit on current chair'
SitOnCurrentChair.does = 'stonehearth:sit_on_current_chair'
SitOnCurrentChair.args = {
   chair = Entity,
}
SitOnCurrentChair.version = 2
SitOnCurrentChair.priority = 1

function SitOnCurrentChair:start_thinking(ai, entity, args)
   local chair = args.chair
   local mount_component = chair:add_component('stonehearth:mount')
   if mount_component:get_user() == entity then
      ai:set_think_output()
   end
end

function SitOnCurrentChair:run(ai, entity, args)
end

function SitOnCurrentChair:stop(ai, entity, args)
   local chair = args.chair

   if chair:is_valid() then
      local mount_component = chair:add_component('stonehearth:mount')
      mount_component:dismount()
   end
end

return SitOnCurrentChair
