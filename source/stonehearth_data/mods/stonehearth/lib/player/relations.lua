local Entity = _radiant.om.Entity

local Relations = radiant.class()
local log = radiant.log.create_logger('faction')

Relations.HOSTILE  = 'hostile'
Relations.NEUTRAL  = 'neutral'
Relations.FRIENDLY = 'friendly'

-- used for sorting 
local HOSTILITY = {
   [Relations.FRIENDLY] = 0,
   [Relations.NEUTRAL]  = 1,
   [Relations.HOSTILE]  = 2,
}

local function is_neutral_player(player_id)
   checks('?string')
   return player_id == nil or 
          player_id == '' or
          player_id == 'critters'
end

-- get the key to use to look up the amenity between players `player_a`
-- and `player_b`
--
local function get_amenity_key(player_a, player_b)
   checks('string', 'string')

   -- sort before checking the amenity table...
   if player_b < player_a then
      return get_amenity_key(player_b, player_a)
   end
   return player_a .. ',' .. player_b
end

-- pick the most hostile amenity between `rep_a` and `rep_b`
--
local function most_hostile(rep_a, rep_b)
   checks('string', 'string')
   return HOSTILITY[rep_a] > HOSTILITY[rep_b] and rep_a or rep_b
end

function Relations:__init(amenity_map)
   checks('self', 'table')
   self._amenity_map = amenity_map
end

-- get the amenity between `player_a` and `player_b`
--
function Relations:get_amenity(player_a, player_b)
   checks('self', '?string', '?string')
   
   -- always friendly with self.
   if player_a == player_b then
      return Relations.FRIENDLY
   end

   -- if either player is one of our magic 'neutral' players, it
   -- doesn't matter what the other guy is.
   if is_neutral_player(player_a) or is_neutral_player(player_b) then
      return Relations.NEUTRAL
   end

   -- see if anything's happened to change the default...
   local key = get_amenity_key(player_a, player_b)
   local amenity = self._amenity_map[key]
   if amenity then
      return amenity
   end

   -- no?  pick the worst default
   amenity = Relations.FRIENDLY
   amenity = most_hostile(amenity, self._amenity_map[get_amenity_key(player_a, 'strangers')] or amenity)
   amenity = most_hostile(amenity, self._amenity_map[get_amenity_key(player_b, 'strangers')] or amenity)

   return amenity
end

function Relations:set_amenity_to_strangers(player_a, amenity)
   checks('self', 'string', 'string')
   self:set_amenity(player_a, 'strangers', amenity)
end

-- override the default calcuation for the amenity between two players with
-- the specified `amenity`.  setting the amenity to nil will revert back
-- to the computed behavior.
--
function Relations:set_amenity(player_a, player_b, amenity)
   checks('self', 'string', 'string', 'string')

   log:debug('changing amenity between %s and %s to %s', player_a, player_b, amenity)
   local key = get_amenity_key(player_a, player_b)
   self._amenity_map[key] = amenity
end

-- return whether or not the relevant parties are hostle. May pass either strings or entities.
function Relations:are_players_hostile(party_a, party_b)
   local player_a = radiant.entities.get_player_id(party_a)
   local player_b = radiant.entities.get_player_id(party_b)
   local amenity = self:get_amenity(player_a, player_b)
   return amenity == Relations.HOSTILE
end

-- return whether or not the relevant parties are hostle. May pass either strings or entities.
function Relations:are_players_friendly(party_a, party_b)
   local player_a = radiant.entities.get_player_id(party_a)
   local player_b = radiant.entities.get_player_id(party_b)
   local amenity = self:get_amenity(player_a, player_b)
   return amenity == Relations.FRIENDLY
end

return Relations

