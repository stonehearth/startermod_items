local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local Mission = require 'services.server.game_master.controllers.missions.mission'

local RaidStockpilesMission = class()
mixin_class(RaidStockpilesMission, Mission)

function RaidStockpilesMission:activate()
   if self._sv.update_orders_timer then
      self._sv.update_orders_timer:bind(function()
            self:_update_party_orders()
         end)
   end
   if self._sv.sighted_bulletin_data then
      self:_listen_for_sighted()
   end
end

function RaidStockpilesMission:can_start(ctx, info)
   assert(ctx)
   assert(ctx.enemy_location)
   assert(info)
   assert(info.npc_player_id)

   -- don't raid the enemy stockpiles if there's no room in ours
   if info.require_free_stockpile_space then
      local friendly_stockpiles = stonehearth.inventory:get_inventory(info.npc_player_id)
                                                            :get_all_stockpiles()
      if friendly_stockpiles then
         for _, stockpile in pairs(friendly_stockpiles) do
            local full = stockpile:get_component('stonehearth:stockpile')
                                    :is_full()
            if not full then
               return true
            end
         end
      end
      return false
   end
   return true
end

function RaidStockpilesMission:start(ctx, info)
   self._sv.ctx = ctx
   self._sv.info = info
   self._sv.party = self:_create_party(ctx, info)

   if info.sighted_bulletin then
      self._sv.sighted_bulletin_data = info.sighted_bulletin
      self.__saved_variables:mark_changed()
      self:_listen_for_sighted()
   end
   self:_update_party_orders()
end

function RaidStockpilesMission:stop()
   if self._sighted_listener then
      self._sighted_listener:destroy()
      self._sighted_listener = nil
   end   
   if self._stockpile_listener then
      self._stockpile_listener:destroy()
      self._stockpile_listener = nil
   end
   if self._sv.update_orders_timer then
      self._sv.update_orders_timer:destroy()
      self._sv.update_orders_timer = nil
      self.__saved_variables:mark_changed()
   end
   local bulletin = self._sv.sighted_bulletin
   if bulletin then
      stonehearth.bulletin_board:remove_bulletin(bulletin)
      self._sv.sighted_bulletin = nil
      self.__saved_variables:mark_changed()
   end   
end

function RaidStockpilesMission:_listen_for_sighted()
   local ctx = self._sv.ctx
   local population = stonehearth.population:get_population(ctx.player_id)
   if population then
      self._sighted_listener = radiant.events.listen(population, 'stonehearth:population:new_threat', self, self._on_player_new_threat)
   end
end

function RaidStockpilesMission:_on_player_new_threat(evt)
   local party = self._sv.party
   if party and not self._sv.sighted_bulletin then
      for id, member in party:each_member() do
         if id == evt.entity_id then
            self:_create_sighted_bulletin(member)
            self._sighted_listener:destroy()
            self._sighted_listener = nil
         end
      end
   end
end

function RaidStockpilesMission:_create_sighted_bulletin(party_member)
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

function RaidStockpilesMission:_update_party_orders()
   local ctx = self._sv.ctx
   local info = self._sv.info
   
   if self._sv.update_orders_timer then
      self._sv.update_orders_timer:destroy()
      self._sv.update_orders_timer = nil
      self.__saved_variables:mark_changed()
   end   

   local raid_stockpile = self:_find_closest_stockpile(ctx.enemy_location, ctx.player_id)
   if raid_stockpile then
      local friendly_stockpiles = stonehearth.inventory:get_inventory(info.npc_player_id)
                                                            :get_all_stockpiles()
      if friendly_stockpiles and next(friendly_stockpiles) then
         self:_raid_stockpile(raid_stockpile, friendly_stockpiles)
         return
      end
   end
   self:_pick_random_spot(ctx)
end

function RaidStockpilesMission:_raid_stockpile(raid_stockpile, friendly_stockpiles)
   self._raiding_stockpile = raid_stockpile
   if self._stockpile_listener then
      self._stockpile_listener:destroy()
      self._stockpile_listener = nil
   end
   self._stockpile_listener = radiant.events.listen(self._raiding_stockpile, 'stonehearth:stockpile:item_removed', self, self._check_stockpile)

   -- the formation should be the center of the stockpile
   local location = raid_stockpile:get_component('stonehearth:stockpile')
                                       :get_bounds()
                                          :get_centroid()   

   local party = self._sv.party
   party:place_banner(party.ATTACK, location, 0)
   party:cancel_tasks()
   for _, stockpile in pairs(friendly_stockpiles) do
      party:add_task('stonehearth:steal_items_in_stockpile', {
                  from_stockpile = raid_stockpile,
                  to_stockpile = stockpile,
               })
               :set_priority(stonehearth.constants.priorities.party.RAID_STOCKPILE)
               :start()
      party:add_task('stonehearth:destroy_items_in_stockpile', {
                  stockpile = raid_stockpile,
               })
               :set_priority(stonehearth.constants.priorities.party.RAID_STOCKPILE)
               :start()
   end
end

function RaidStockpilesMission:_check_stockpile(e)
   if e.stockpile == self._raiding_stockpile then
      local items = e.stockpile:get_component('stonehearth:stockpile')
                                 :get_items()
      if not next(items) then
         if self._stockpile_listener then
            self._stockpile_listener:destroy()
            self._stockpile_listener = nil
         end
         self:_update_party_orders()
      end
   end 
end

function RaidStockpilesMission:_pick_random_spot(ctx)
   if self._sv.update_orders_timer then
      self._sv.update_orders_timer:destroy()
      self._sv.update_orders_timer = nil
      self.__saved_variables:mark_changed()
   end   
   self._sv.update_orders_timer = stonehearth.calendar:set_timer('4h', function()
         self:_update_party_orders()
      end)
   self.__saved_variables:mark_changed()

   local town = stonehearth.town:get_town(self._sv.ctx.player_id)
   if not town then
      return
   end
   local camp_standard = town:get_banner()   
   if not camp_standard then
      return
   end
   local location = radiant.entities.get_world_grid_location(camp_standard)
   if not location then
      return
   end
   
   local location = location + Point3(rng:get_int(-25, 25), 0, rng:get_int(-25, 25))
   local party = self._sv.party
   party:place_banner(party.ATTACK, location, 0)
   party:cancel_tasks()
end

return RaidStockpilesMission
