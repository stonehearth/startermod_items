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
   local chair = args.chair
   local mount_component = chair:add_component('stonehearth:mount')
   mount_component:mount(entity)
end

function SitOnChairAdjacent:stop(ai, entity, args)
   local chair = args.chair

   if chair:is_valid() then
      local mount_component = chair:add_component('stonehearth:mount')
      if mount_component:is_in_use() then
         assert(mount_component:get_user() == entity)
         mount_component:dismount()
      end
   end
end

return SitOnChairAdjacent
