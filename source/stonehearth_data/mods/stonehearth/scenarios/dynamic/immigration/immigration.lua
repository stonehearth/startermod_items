local Immigration = class()
local rng = _radiant.csg.get_default_rng()
local Point3 = _radiant.csg.Point3

--[[
   Immigration Synopsis
   If your settlement meets certain conditions, cool people might want to join up. 
   The profession of the person who joins may change as your town's happiness score increases.

   Details: the professions available are defined in immigration.json. Each profession has
   a value (1 is low, N is high) and a # of shares. The professions are entered into a table.
   Professions with high share values appear in the table multiple times. 

   When the scenario runs, we check the town's happines score. The score determines the number
   of "random drawings" we do out of the profession table, so happy towns will have lots of drawings.
   The profession with the highest value is selected to join the town. 

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

function Immigration:__init()
   --TODO: Test that the notice sticks around even after a save

   self._immigration_data =  radiant.resources.load_json('stonehearth:scenarios:immigration')
   
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

--TODO: get the player ID from the caller
function Immigration:start()
   --TODO: should we delay the caravan's appearance? 
   --Though we'd rather not have it happen immediately, we could also
   --handle that with a curve

   --Make the new citizen
   local target_profession = self:_pick_immigrant_profession()
   --TODO: get this from elsewhere
   local player_id = 'player_1'
   local pop = stonehearth.population:get_population(player_id)
   local citizen = pop:create_new_citizen()
   pop:promote_citizen(citizen, target_profession)

   --Compose the message
   local message = self:_compose_message(target_profession, citizen)

   --Get the data ready to send to the bulletin
   local notice = {
      title = self._immigration_data.title,
      message = message
   }

   --TODO: send bulletin when Albert has his new API

   --TODO: make this happen after the player agrees to having the dude appear
   self:place_citizen(citizen)
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

function Immigration:_compose_message(profession, citizen)
   local message_index = rng:get_int(1, #self._immigration_data.messages)
   local message = self._immigration_data.messages[message_index]
   local outcome_statement = self._immigration_data.outcome_statement
   local citizen_name = radiant.entities.get_name(citizen)
   local profession_name = radiant.resources.load_json(profession).name
   local town_name = stonehearth.town:get_town(radiant.entities.get_player_id(citizen)):get_town_name()


   outcome_statement = string.gsub(outcome_statement, '__profession__', profession_name)
   outcome_statement = string.gsub(outcome_statement, '__name__', citizen_name)
   outcome_statement = string.gsub(outcome_statement, '__town_name__', town_name)
   return message .. ' ' .. outcome_statement
end

function Immigration:place_citizen(citizen)
   local player_id = radiant.entities.get_player_id(citizen)
   local town = stonehearth.town:get_town(player_id)
   local banner_entity = town:get_banner()

   --TODO: eventually, the trader will drop this when standing near the banner, but for now...
   local target_location = radiant.entities.pick_nearby_location(banner_entity, 3)

   radiant.terrain.place_entity(citizen, target_location)

   --TODO: attach particle effect
end

return Immigration