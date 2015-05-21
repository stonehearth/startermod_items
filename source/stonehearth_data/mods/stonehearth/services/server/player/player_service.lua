local Entity = _radiant.om.Entity

local PlayerService = class()
local log = radiant.log.create_logger('player')

PlayerService.HOSTILE  = 'hostile'
PlayerService.NEUTRAL  = 'neutral'
PlayerService.FRIENDLY = 'friendly'

-- used for sorting 
local HOSTILITY = {
   [PlayerService.FRIENDLY] = 0,
   [PlayerService.NEUTRAL]  = 1,
   [PlayerService.HOSTILE]  = 2,
}

function PlayerService:initialize()
   self._sv = self.__saved_variables:get_data() 
   if not self._sv.players then
      self._sv.players = {}
      self._sv.amenity_map = {}
      self:_create_npc_players()
   end
end

function PlayerService:activate()
   -- amenity_map was added for Alpha 10b.  If it's not there, stick it in.
   if not self._sv.amenity_map then
      self._sv.amenity_map = {}
      self.__saved_variables:mark_changed()
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

-- pick the most hostile amenity between `rep_a` and `rep_b`
--
function PlayerService:_most_hostile(rep_a, rep_b)
   checks('self', 'string', 'string')
   return HOSTILITY[rep_a] > HOSTILITY[rep_b] and rep_a or rep_b
end

-- get the key to use to look up the amenity between players `player_a`
-- and `player_b`
--
function PlayerService:_get_amenity_key(player_a, player_b)
   checks('self', 'string', 'string')

   -- sort before checking the amenity table...
   if player_b < player_a then
      return self:_get_amenity_key(player_b, player_a)
   end
   return player_a .. ',' .. player_b
end

-- get the amenity between `player_a` and `player_b`
--
function PlayerService:_get_amenity(player_a, player_b)
   checks('self', '?string', '?string')
   
   -- always friendly with self.
   if player_a == player_b then
      return PlayerService.FRIENDLY
   end

   -- if either player is one of our magic 'neutral' players, it
   -- doesn't matter what the other guy is.
   if self:_is_neutral_player(player_a) or self:_is_neutral_player(player_b) then
      return PlayerService.NEUTRAL
   end

   -- see if anything's happened to change the default...
   local key = self:_get_amenity_key(player_a, player_b)
   local amenity = self._sv.amenity_map[key]
   if amenity then
      return amenity
   end

   -- no?  pick the worst default
   amenity = PlayerService.FRIENDLY
   local pop = stonehearth.population:get_population(player_a)
   if pop then
      amenity = self:_most_hostile(amenity, pop:get_amenity_to_strangers())
   end
   pop = stonehearth.population:get_population(player_b)
   if pop then
      amenity = self:_most_hostile(amenity, pop:get_amenity_to_strangers())
   end

   return amenity
end

-- override the default calcuation for the amenity between two players with
-- the specified `amenity`.  setting the amenity to nil will revert back
-- to the computed behavior.
--
function PlayerService:set_amenity(player_a, player_b, amenity)
   checks('self', 'string', 'string', 'string')

   log:debug('changing amenity between %s and %s to %s', player_a, player_b, amenity)
   local key = self:_get_amenity_key(player_a, player_b)
   self._sv.amenity_map[key] = amenity
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
   local player_a = radiant.entities.get_player_id(party_a)
   local player_b = radiant.entities.get_player_id(party_b)
   local amenity = self:_get_amenity(player_a, player_b)
   return amenity == PlayerService.HOSTILE
end

-- return whether or not the relevant parties are hostle. May pass either strings or entities.
function PlayerService:are_players_friendly(party_a, party_b)
   local player_a = radiant.entities.get_player_id(party_a)
   local player_b = radiant.entities.get_player_id(party_b)
   local amenity = self:_get_amenity(player_a, player_b)
   return amenity == PlayerService.FRIENDLY
end

-- we'll use a mapping table later to determine alliances / hostilities
-- xxx: make all "critters" "animals" or add a "critters" player -- tony
function PlayerService:_is_neutral_player(player_id)
   checks('self', '?string')
   return player_id == nil or 
          player_id == '' or
          player_id == 'critters'
end

return PlayerService
