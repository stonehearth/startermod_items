local Point3 = _radiant.csg.Point3

local Mission = class()

function Mission:_create_party(ctx, info)
   assert(ctx.enemy_location)
   assert(ctx.enemy_player_id)
   assert(info.offset)
   assert(info.members)
   
   local origin = ctx.enemy_location
   local population = stonehearth.population:get_population(ctx.enemy_player_id)

   local offset = Point3(info.offset.x, info.offset.y, info.offset.z)

   -- xxx: "enemy" here should be "npc"
   local party = stonehearth.unit_control:get_controller(ctx.enemy_player_id)
                                             :create_party()
   for name, job in pairs(info.members) do
      local member = population:create_new_citizen()
      
      member:add_component('stonehearth:job')
                  :promote_to(job)
      
      party:add_member(member)

      local offset = Point3(info.offset.x, info.offset.y, info.offset.z)
      radiant.terrain.place_entity(member, origin + offset)
   end
   return party
end

function Mission:_find_closest_stockpile(player_id)
   return nil
end

function Mission:_find_closest_structure(player_id)
   return nil
end

return Mission

