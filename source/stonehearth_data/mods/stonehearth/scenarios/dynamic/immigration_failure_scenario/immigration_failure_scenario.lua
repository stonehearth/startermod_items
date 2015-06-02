local ImmigrationFailure = class()
local rng = _radiant.csg.get_default_rng()
local Point3 = _radiant.csg.Point3

--[[
   Spawns when a traveller comes by your town and your town is so poor that 
   the traveller would rather give you food than join you. 
]]

--- Returns true if we're so poor that the traveller feels obliged to help
function ImmigrationFailure:can_spawn()
   local food_threshold = self._immigration_data.edible_threshold
   local score_data = stonehearth.score:get_scores_for_player(self._sv.player_id):get_score_data()
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

   self._sv.notice_data = {}
   self._sv.immigration_bulletin = nil
   self:_load_trade_data()
end

function ImmigrationFailure:restore()
   self:_load_trade_data()

   --If we made an expire timer then we're waiting for the player to acknowledge the traveller
   --Start a timer that will expire at that time
   if self._sv.timer then
      self._sv.timer:bind(function()
            self:_timer_callback()
         end)
   end
end

function ImmigrationFailure:_load_trade_data()
   --Read in the relevant data
   self._immigration_data = radiant.resources.load_json('stonehearth:scenarios:immigration_failure').scenario_data
   self._reward_table = {}
   for reward_uri, data in pairs(self._immigration_data.rewards) do
      table.insert(self._reward_table, reward_uri)
   end
end

-- Pick the charity item and prep the message
--TODO: should we delay traveller's appearance to during the daytime?
function ImmigrationFailure:start()
   local target_reward_index = rng:get_int(1, #self._reward_table)
   local reward_uri = self._reward_table[target_reward_index]
   local reward_data = self._immigration_data.rewards[reward_uri]
   local num_items = rng:get_int(reward_data.min, reward_data.max)

   local title = self._immigration_data.title
   local message_index = rng:get_int(1, #self._immigration_data.messages)
   local message = self._immigration_data.messages[message_index]

   --Enhance message with object name
   local reward_info = radiant.resources.load_json(reward_uri)
   local reward_name = reward_info.components.unit_info.name
   local statement = self._immigration_data.outcome_statement
   statement = string.gsub(statement, '__num_items__', num_items)
   statement = string.gsub(statement, '__item_name__', reward_name)

   message = message .. statement

   self._sv.notice_data = {
      title = title, 
      message = message, 
      reward = reward_uri, 
      quantity = num_items, 
   }

   --Send the notice to the bulletin service. Should be parametrized by player
   self._sv.immigration_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
           :set_ui_view('StonehearthGenericBulletinDialog')
           :set_callback_instance(self)
           :set_data({
               title = title,
               message = message,
               accepted_callback = "_on_accepted",
               declined_callback = "_on_declined",
           })

   --Allow the notice to timeout after the designated time, after which point we remove it. 
   local wait_duration = self._immigration_data.expiration_timeout
   self:_create_timer(wait_duration)
   
   --Add something to the event log:
   stonehearth.events:add_entry(title .. ': ' .. message)
end

function ImmigrationFailure:_timer_callback()
   if self._sv.immigration_bulletin then
      local bulletin_id = self._sv.immigration_bulletin:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      self:_stop_timer()
   end
end

function ImmigrationFailure:_create_timer(duration)
   self._sv.timer = stonehearth.calendar:set_timer("ImmigrationFailure remove bulletin", duration, function()
         self:_timer_callback()
      end)
   self.__saved_variables:mark_changed()
end

--- Once the user has acknowledged the bulletin, add the target reward beside the banner
function ImmigrationFailure:acknowledge()
   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return
   end

   local uris = {}
   uris[self._sv.notice_data.reward] = self._sv.notice_data.quantity

   --TODO: attach a brief particle effect to the new stuff
   radiant.entities.spawn_items(uris, drop_origin, 1, 3, { owner = self._sv.player_id })
end

--- Only actually spawn the object after the user clicks OK
-- TODO: verify the bulletin actually goes away?
function ImmigrationFailure:_on_accepted()
   self:_stop_timer()
   self:acknowledge()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')

end

function ImmigrationFailure:_on_declined()
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function ImmigrationFailure:_stop_timer()
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
      self.__saved_variables:mark_changed()
   end
end

return ImmigrationFailure