local Relations = require 'lib.player.relations'

local Entity = _radiant.om.Entity

local PlayerService = class()
local log = radiant.log.create_logger('player')

function PlayerService:initialize()
   self._sv = self.__saved_variables:get_data() 
   
   if not self._sv.amenity_map then
      self._sv.amenity_map = {}
   end
   self._player_relations = Relations(self._sv.amenity_map)
   
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

      local pop = stonehearth.population:add_population(player_id, kingdom)      
      local amenity = pop:get_amenity_to_strangers()
      
      pop:set_is_npc(is_npc)
      self._player_relations:set_amenity_to_strangers(player_id, amenity)

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
   local player_id = radiant.entities.get_player_id(arg1)
   local pop = stonehearth.population:get_population(player_id)
   if not pop then
      log:error('no population for player "%s" in :is_npc()', player_id)
      return
   end

   local is_npc = pop:is_npc()
   log:spam(':is_npc() returning %s', tostring(is_npc))
   return is_npc
end

-- override the default calcuation for the amenity between two players with
-- the specified `amenity`.  setting the amenity to nil will revert back
-- to the computed behavior.
--
function PlayerService:set_amenity(player_a, player_b, amenity)
   checks('self', 'string', 'string', 'string')
   
   self._player_relations:set_amenity(player_a, player_b, amenity)
   self.__saved_variables:mark_changed()

   local pop = stonehearth.population:get_population(player_a)
   if pop then
      radiant.events.trigger_async(pop, 'stonehearth:amenity_changed', { other_player = player_b })
   end

   pop = stonehearth.population:get_population(player_b)
   if pop then
      radiant.events.trigger_async(pop, 'stonehearth:amenity_changed', { other_player = player_a })
   end
end

-- return whether or not the relevant parties are hostle. May pass either strings or entities.
function PlayerService:are_players_hostile(party_a, party_b)
   return self._player_relations:are_players_hostile(party_a, party_b)
end

-- return whether or not the relevant parties are hostle. May pass either strings or entities.
function PlayerService:are_players_friendly(party_a, party_b)
   return self._player_relations:are_players_friendly(party_a, party_b)
end

return PlayerService
