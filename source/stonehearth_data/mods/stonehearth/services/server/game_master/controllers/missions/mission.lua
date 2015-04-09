local game_master_lib = require 'lib.game_master.game_master_lib'

local Point3 = _radiant.csg.Point3

local Mission = class()

function Mission:_create_party(ctx, info)
   assert(ctx)
   assert(ctx.enemy_location)
   assert(info.offset)
   assert(info.members)

   local npc_player_id = ctx.npc_player_id or info.npc_player_id
   assert (npc_player_id)
   
   local origin = ctx.enemy_location
   local population = stonehearth.population:get_population(npc_player_id)

   local offset = Point3(info.offset.x, info.offset.y, info.offset.z)

   -- xxx: "enemy" here should be "npc"
   local party = stonehearth.unit_control:get_controller(npc_player_id)
                                             :create_party()
   for name, info in pairs(info.members) do
      local members = game_master_lib.create_citizens(population, info, origin + offset, ctx)
      for id, member in pairs(members) do
         party:add_member(member)
      end
   end
   return party
end

function Mission:_find_closest_stockpile(origin, player_id)
   assert(origin)
   assert(player_id)

   local inventory = stonehearth.inventory:get_inventory(player_id)
   if not inventory then
      return nil
   end
   local stockpiles = inventory:get_all_stockpiles()
   if not stockpiles then
      return nil
   end

   local best_dist, best_stockpile
   for id, stockpile in pairs(stockpiles) do
      local location = radiant.entities.get_world_grid_location(stockpile)
      local sc = stockpile:get_component('stonehearth:stockpile')
      local items = sc:get_items()
      if next(items) then
         local cube = sc:get_bounds()
         local dist = cube:distance_to(origin)
         if not best_dist or dist < best_dist then
            best_dist = dist
            best_stockpile = stockpile
         end
      end
   end

   return best_stockpile
end

function Mission:_find_closest_structure(player_id)
   return nil
end

return Mission

