local PlayerService = class()

function PlayerService:initialize()
   self._sv = self.__saved_variables:get_data() 
   if not self._sv.players then
      self._sv.players = {}
      self:_create_npc_players()
   end
end

function PlayerService:add_player(player_id, kingdom)
   local town = stonehearth.town:get_town(player_id)
   if not town then
      stonehearth.town:add_town(player_id)
      stonehearth.inventory:add_inventory(player_id)
      stonehearth.population:add_population(player_id, kingdom)
      stonehearth.terrain:get_visible_region(player_id)
      stonehearth.terrain:get_explored_region(player_id)

      self._sv.players[player_id] = {
         kingdom = kingdom
      }
      self.__saved_variables:mark_changed()
   end 
end

function PlayerService:_create_npc_players()
   local players = radiant.resources.load_json('stonehearth:data:npc_index')
   if players then
      for player_id, info in pairs(players) do
         self:add_player(player_id, info.kingdom)
      end
   end
end

return PlayerService
