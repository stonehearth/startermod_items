local ImmigrationFailure = class()
local rng = _radiant.csg.get_default_rng()
local Point3 = _radiant.csg.Point3

--[[
   Spawns when a traveller comes by your town and your town is so poor that 
   the traveller would rather give you food than join you. 
]]

--- Returns true if we're so poor that the traveller feels obliged to help
--  TODO: account for which player; currently THIS IS HARDCODED!!
function ImmigrationFailure:can_spawn()
   local food_threshold = self._immigration_data.edible_threshold
   local score_data = stonehearth.score:get_scores_for_player(self._sv.player_id)
   local last_edible_score = 0
   if score_data and score_data.resources and score_data.resources.edibles then
      last_edible_score = score_data.resources.edibles
   end
   local is_really_poor = last_edible_score < food_threshold
   return is_really_poor
end

function ImmigrationFailure:initialize()
   --TODO: pass this in from dmservice
   self._sv.player_id = 'player_1'

   --TODO: test that this works across saving/loading
   self._sv.notice_data = {}
   self._sv.immigration_bulletin = nil
   self:_load_trade_data()
end

function ImmigrationFailure:restore()
   self:_load_trade_data()
end

function ImmigrationFailure:_load_trade_data()
   --Read in the relevant data
   self._immigration_data = radiant.resources.load_json('stonehearth:scenarios:immigration_failure')
   self._reward_table = {}
   for reward_uri, data in pairs(self._immigration_data.rewards) do
      table.insert(self._reward_table, reward_uri)
   end
end


function ImmigrationFailure:start()
   --TODO: should we delay the caravan's appearance? 
   --Though we'd rather not have it happen immediately, we could also
   --handle that with a curve
   local target_reward_index = rng:get_int(1, #self._reward_table)
   local reward_uri = self._reward_table[target_reward_index]
   local reward_data = self._immigration_data.rewards[reward_uri]
   local num_items = rng:get_int(reward_data.min, reward_data.max)

   local title = self._immigration_data.title
   local message_index = rng:get_int(1, #self._immigration_data.messages)
   local message = self._immigration_data.messages[message_index]

   --Enhance message with object name
   local reward_data = radiant.resources.load_json('stonehearth:berry_basket')
   local reward_name = reward_data.components.unit_info.name
   local statement = self._immigration_data.outcome_statement .. reward_name
   message = message .. statement

   self._sv.notice_data = {
      title = title, 
      message = message, 
      reward = reward_uri, 
      quantity = num_items, 
   }

   --Send the notice to the bulletin service. Should be parametrized by player
   self._sv.immigration_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
           :set_config('stonehearth:bulletins:immigration_failure')
           :set_callback_instance(self)
           :set_data({
               title = title,
               message = message,
           })

   --TODO: allow the notice to timeout after a while, after which point we remove it. 

   --Add something to the event log:
   stonehearth.events:add_entry(title .. ': ' .. message)
end

--- Once the user has acknowledged the bulletin, add the target reward beside the banner
function ImmigrationFailure:acknowledge()
   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner_entity = town:get_banner()

   --TODO: eventually, the trader will drop this when standing near the banner, but for now...
   local target_location = radiant.entities.pick_nearby_location(banner_entity, 3)

   local gift = radiant.entities.create_entity(self._sv.notice_data.reward)   
   radiant.terrain.place_entity(gift, target_location)

   --TODO: attach a brief particle effect to the new stuff

end

--- Only actually spawn the object after the user clicks OK
function ImmigrationFailure:_on_accepted()
   self:acknowledge()
end

function ImmigrationFailure:_on_declined()
   --Do nothing
end

return ImmigrationFailure