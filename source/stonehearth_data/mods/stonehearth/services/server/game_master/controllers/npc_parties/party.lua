local Point3 = _radiant.csg.Point3

local Party = class()

function Party:_create_party(ctx, info)
   assert(ctx.enemy_location)
   assert(ctx.enemy_player_id)
   assert(info.offset)
   assert(info.members)
   
   local origin = ctx.enemy_location
   local population = stonehearth.population:get_population(ctx.enemy_player_id)

   local offset = Point3(info.offset.x, info.offset.y, info.offset.z)
   local members = {}
   self._sv.members = members
   for name, job in pairs(info.members) do
      local member = population:create_new_citizen()
      
      member:add_component('stonehearth:job')
                  :promote_to(job)
      
      members[name] = member
      
      local offset = Point3(info.offset.x, info.offset.y, info.offset.z)
      radiant.terrain.place_entity(member, origin + offset)
   end
end

return Party

