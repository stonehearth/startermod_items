local game_master_lib = require 'lib.game_master.game_master_lib'

local Point3 = _radiant.csg.Point3

local Mission = class()

function Mission:activate()
   if self._sv.sighted_bulletin_data then
      self:_listen_for_sighted()
   end
end

function Mission:can_start(ctx, info)
   return true
end

function Mission:start(ctx, info)
   self._sv.ctx = ctx
   self._sv.info = info

   self._sv.party = self:_create_party(ctx, info)

   if info.sighted_bulletin then
      self._sv.sighted_bulletin_data = info.sighted_bulletin
      self.__saved_variables:mark_changed()
      self:_listen_for_sighted()
   end
end

function Mission:stop()
   if self._sighted_listener then
      self._sighted_listener:destroy()
      self._sighted_listener = nil
   end

   local bulletin = self._sv.sighted_bulletin
   if bulletin then
      stonehearth.bulletin_board:remove_bulletin(bulletin)
      self._sv.sighted_bulletin = nil
      self.__saved_variables:mark_changed()
   end      
end

function Mission:_listen_for_sighted()
   local ctx = self._sv.ctx
   local population = stonehearth.population:get_population(ctx.player_id)
   if population then
      self._sighted_listener = radiant.events.listen(population, 'stonehearth:population:new_threat', self, self._on_player_new_threat)
   end
end

function Mission:_on_player_new_threat(evt)
   local party = self._sv.party
   if party and not self._sv.sighted_bulletin then
      for id, member in party:each_member() do
         if id == evt.entity_id then
            self:_create_sighted_bulletin(member)
            self._sighted_listener:destroy()
            self._sighted_listener = nil
            break
         end
      end
   end
end

function Mission:_create_sighted_bulletin(party_member)
   if not self._sv.sighted_bulletin then
      --Send the notice to the bulletin service.
      local player_id = self._sv.ctx.player_id
      local bulletin_data = self._sv.sighted_bulletin_data
      bulletin_data.zoom_to_entity = party_member

      self._sv.sighted_bulletin = stonehearth.bulletin_board:post_bulletin(player_id)
           :set_type('alert')
           :set_data(bulletin_data)
      self.__saved_variables:mark_changed()
   end
end

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

