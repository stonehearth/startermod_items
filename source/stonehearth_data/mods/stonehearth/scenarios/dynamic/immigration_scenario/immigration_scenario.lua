local Immigration = class()
local rng = _radiant.csg.get_default_rng()
local Point3 = _radiant.csg.Point3

--[[
   Immigration Synopsis
   If your settlement meets certain conditions, cool people might want to join up. 
   The profession of the person who joins may change as your town's happiness score increases.

   Details: the professions available are defined in immigration.json. Each profession has
   a rating (1 is low, N is high) and a # of shares. The professions are entered into a table.
   Professions with high share values appear in the table multiple times. 

   When the scenario runs, we check the town's happines score. The score determines the number
   of "random drawings" we do out of the profession table, so happy towns will have lots of drawings.
   The profession with the highest rating is selected to join the town. 

   This way, the list of professions is extensible, but your changes of getting a rare specialist
   go up as your town's happiness increases.
]]

--Returns true if: 
-- > the town's happiness level is > 4 
-- > there's at least 10 food items
-- > score is at least 100 
-- (all these are set in immigration.json)
function Immigration:can_spawn()
   local immigration_data = self._immigration_data
   local score_data = stonehearth.score:get_scores_for_player('player_1'):get_score_data()

   local happiness_threshold = immigration_data.happiness_threshold
   local happiness_score = 0
   if score_data.happiness and score_data.happiness.happiness then
      happiness_score = score_data.happiness.happiness/10
   end
   if happiness_score < happiness_threshold then
      return false
   end

   local food_threshold = immigration_data.food_threshold
   local available_food = 0
   if score_data.resources and score_data.resources.edibles then
      available_food = score_data.resources.edibles
   end 
   if available_food < food_threshold then
      return false
   end
   
   local score_threshold = immigration_data.score_threshold
   local curr_score = 0
   if score_data.net_worth and score_data.net_worth.total_score then
      curr_score = score_data.net_worth.total_score
   end
   if curr_score < score_threshold then
      return false
   end

   return true
end

function Immigration:initialize()
   --TODO: Test that the notice sticks around even after a save

   self._sv.notice = {}
   --TODO: get this from caller
   self._sv.player_id = 'player_1'
   self:_load_trade_data()
end

function Immigration:restore()
   self:_load_trade_data()

   --If we made an expire timer then we're waiting for the player to acknowledge the traveller
   --Start a timer that will expire at that time
   if self._sv.timer_expiration then
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         local duration = self._sv.timer_expiration - stonehearth.calendar:get_elapsed_time()
         self:_create_timer(duration)
         return radiant.events.UNLISTEN
      end)
   end
end

function Immigration:_load_trade_data()
   self._immigration_data =  radiant.resources.load_json('stonehearth:scenarios:immigration').scenario_data
   
   --Create a table with all the possible professions.
   --Each profession is entered into the table n times, where n is the number of "shares"
   --that profession has. Higher shares means a higher chance of being selected
   self._possibility_table = {}
   for profession, data in pairs(self._immigration_data.professions) do
      for i=0, data.shares do
         table.insert(self._possibility_table, profession)
      end
   end
end

-- Make a citizen and propose his approach
function Immigration:start()

   --Make the new citizen
   local target_profession = self:_pick_immigrant_profession()

   --Compose the message
   local message = self:_compose_message(target_profession)

   --Get the data ready to send to the bulletin
   self._sv.notice = {
      title = self._immigration_data.title,
      message = message, 
      target_profession = target_profession
   }

   --Send the notice to the bulletin service.
   self._sv.immigration_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
           :set_ui_view('StonehearthGenericBulletinDialog')
           :set_callback_instance(self)
           :set_data({
               title = self._immigration_data.title,
               message = message,
               accepted_callback = "_on_accepted",
               declined_callback = "_on_declined",
           })

   --Make sure it times out if we don't get to it
   local wait_duration = self._immigration_data.expiration_timeout
   self:_create_timer(wait_duration)

   --Add something to the event log:
   stonehearth.events:add_entry(self._immigration_data.title .. ': ' .. message)
end

--- Get a random target profession out of the table
-- Make a number of drawings out of the profession table based on the happiness of the town
-- Use the highest rated job as the new job. 
-- @returns profession name
function Immigration:_pick_immigrant_profession()
   local score_data = stonehearth.score:get_scores_for_player('player_1'):get_score_data()
   local happiness_score = 0
   if score_data.happiness and score_data.happiness.happiness then
      happiness_score = score_data.happiness.happiness/10 - 2
   end

   local best_profession = 'stonehearth:professions:worker'
   local best_profession_value = 1
   for i=1, happiness_score do
      local random_index = rng:get_int(1, #self._possibility_table)
      local target_profession = self._possibility_table[random_index]
      local target_profession_value = self._immigration_data.professions[target_profession].rating
      if target_profession_value > best_profession_value then
         best_profession = target_profession
         best_profession_value = target_profession_value
      end
   end
   return best_profession
end

function Immigration:_compose_message(profession)
   local message_index = rng:get_int(1, #self._immigration_data.messages)
   local message = self._immigration_data.messages[message_index]
   local outcome_statement = self._immigration_data.outcome_statement
   local profession_name = radiant.resources.load_json(profession).name
   local town_name = stonehearth.town:get_town(self._sv.player_id):get_town_name()


   outcome_statement = string.gsub(outcome_statement, '__profession__', profession_name)
   outcome_statement = string.gsub(outcome_statement, '__town_name__', town_name)
   return message .. ' ' .. outcome_statement
end

function Immigration:place_citizen(citizen)
   local spawn_point = stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity(citizen, 80)
   if not spawn_point then
      --Just use a place near the banner
      local town = stonehearth.town:get_town(self._sv.player_id)
      local banner_entity = town:get_banner()
      spawn_point = radiant.entities.pick_nearby_location(banner_entity, 3)
   end

   radiant.terrain.place_entity(citizen, spawn_point)

   --TODO: attach particle effect
end

--- Only actually spawn the object after the user clicks OK
function Immigration:_on_accepted()
   local pop = stonehearth.population:get_population(self._sv.player_id)
   local citizen = pop:create_new_citizen()
   pop:promote_citizen(citizen, self._sv.notice.target_profession)

   --If they have equipment, put it on them
   if self._immigration_data.professions[self._sv.notice.target_profession].equipment then
      local equipment_uri = self._immigration_data.professions[self._sv.notice.target_profession].equipment
      local equipment_entity = radiant.entities.create_entity(equipment_uri)
      local equipment_component = citizen:add_component('stonehearth:equipment')
      equipment_component:equip_item(equipment_entity, 'immigrant_equipment')

      --Hack: remove glow?
       equipment_entity:remove_component('effect_list')
   end

   self:place_citizen(citizen)
   if self._timer then
      self._timer:destroy()
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function Immigration:_on_declined()
   if self._timer then
      self._timer:destroy()
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function Immigration:_create_timer(duration)
   self._timer = stonehearth.calendar:set_timer(duration, function() 
      if self._sv.immigration_bulletin then
         local bulletin_id = self._sv.immigration_bulletin:get_id()
         stonehearth.bulletin_board:remove_bulletin(bulletin_id)
         self:_stop_timer()
      end
   end)
   self._sv.timer_expiration = self._timer:get_expire_time()
end

function Immigration:_stop_timer()
   self._timer = nil
   self._sv.timer_expiration = nil
end


return Immigration