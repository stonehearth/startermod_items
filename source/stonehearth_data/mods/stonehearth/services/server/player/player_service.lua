local Entity = _radiant.om.Entity

local PlayerService = class()
local log = radiant.log.create_logger('player')

function PlayerService:initialize()
   self._sv = self.__saved_variables:get_data() 
   if not self._sv.players then
      self._sv.players = {}
      self:_create_npc_players()
   end
end

function PlayerService:add_player(player_id, kingdom, options)
   local is_npc = not not (options and options.is_npc)

   local town = stonehearth.town:get_town(player_id)
   if not town then
      stonehearth.job:add_player(player_id)
      stonehearth.town:add_town(player_id)
      stonehearth.inventory:add_inventory(player_id)
      stonehearth.population:add_population(player_id, kingdom)
                                 :set_is_npc(is_npc)

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
         self:add_player(player_id, info.kingdom, {
               is_npc = true
            })
      end
   end
end

function PlayerService:is_npc(arg1)
   local player_id
   if radiant.util.is_a(arg1, Entity) then
      player_id = radiant.entities.get_player_id(arg1)
   elseif radiant.util.is_a(arg1, 'string') then
      player_id = arg1
   else
      log:error('unexpected arg1 "%s" in :is_npc().', tostring(arg1))
      return
   end

   local pop = stonehearth.population:get_population(player_id)
   if not pop then
      log:error('no population for player "%s" in :is_npc()', player_id)
      return
   end

   local is_npc = pop:is_npc()
   log:spam(':is_npc() returning %s', tostring(is_npc))
   return is_npc
end

return PlayerService
