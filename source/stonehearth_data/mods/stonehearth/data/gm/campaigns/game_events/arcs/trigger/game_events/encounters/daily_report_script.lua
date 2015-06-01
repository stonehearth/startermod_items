--[[
   Immigration Synopsis

   Once a day, we run this scenario. It pops up a dialog showing your settlemet's core stats. 
   If your settlement meets certain conditions, cool people might want to join up. 

   The people who join are now always workers. 
]]

local Immigration = class()

function Immigration:initialize()
   --self._sv.notice = {}
   --self._sv.player_id = 'player_1'
   --self:_load_trade_data()
end

function Immigration:restore()
   --self:_load_trade_data()

   --If we made an expire timer then we're waiting for the player to acknowledge the traveller
   --Start a timer that will expire at that time
   if self._sv.timer then
      self._sv.timer:bind(function()
            self:_timer_callback()
         end)
   end
end


function Immigration:start(ctx, data)
   self._sv.player_id = ctx.player_id
   self._sv.immigration_data = data

   --TODO: remove when we have better perf!
   --For Steam EA, don't spawn if we have more than 40 people
   local num_citizens = stonehearth.population:get_population_size(self._sv.player_id)
   if num_citizens > self._sv.immigration_data.max_citizens then
      return
   end

   --Show a bulletin with food/morale/net worth stats
   local message, success = self:_compose_town_report()
   if success then
      self._sv.immigration_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
         :set_ui_view('StonehearthImmigrationReportDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._sv.immigration_data.update_title,
            message = message,
            conclusion = self._sv.immigration_data.conclusion_positive,
            accepted_callback = "_on_accepted",
            declined_callback = "_on_declined",
         })
   else
      self._sv.immigration_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
         :set_ui_view('StonehearthImmigrationReportDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._sv.immigration_data.update_title,
            message = message,
            conclusion = self._sv.immigration_data.conclusion_negative,
            ok_callback = "_on_declined",
         })
   end

   --Make sure it times out if we don't get to it
   local wait_duration = self._sv.immigration_data.expiration_timeout
   self:_create_timer(wait_duration)
end

function Immigration:_compose_town_report()
   local town_name = stonehearth.town:get_town(self._sv.player_id):get_town_name()
   local population_line = self._sv.immigration_data.town_line
   population_line = string.gsub(population_line, '__town_name__', town_name)
   local num_citizens = stonehearth.population:get_population_size(self._sv.player_id)
   population_line = population_line .. num_citizens
   local summation = self:_eval_requirement(num_citizens)
   local date = stonehearth.calendar:format_date()
   date = (date:gsub("^%l", string.upper))

   local message = {
      date = date, 
      town_name = town_name,
      town_size = num_citizens,
      food_data = summation.food_data, 
      morale_data = summation.morale_data, 
      net_worth_data = summation.net_worth_data

   }
   local success = summation.success
   return message, success
end

function Immigration:_eval_requirement(num_citizens)
   --TODO: the score data should come from all food, not just food in stockpiles
   local score_data = stonehearth.score:get_scores_for_player(self._sv.player_id):get_score_data()

   --Get data for food
   local available_food = 0
   if score_data.resources and score_data.resources.edibles then
      available_food = score_data.resources.edibles
   end 
   local food_success, food_data = self:_find_requirments_by_type_and_pop(available_food, 'food', num_citizens)

   --Get data for morale
   local morale_score = 0
   if score_data.happiness and score_data.happiness.happiness then
      morale_score = score_data.happiness.happiness/10
   end
   local morale_success, morale_data = self:_find_requirments_by_type_and_pop(morale_score, 'morale', num_citizens) 
   morale_data.available =  radiant.math.round(morale_data.available*10)*0.1

   --Get data for net worth
   local curr_score = 0
   if score_data.net_worth and score_data.net_worth.total_score then
      curr_score = score_data.net_worth.total_score
   end
   local net_worth_success, net_worth_data = self:_find_requirments_by_type_and_pop(curr_score, 'net_worth', num_citizens)

   local summation = {
      food_data = food_data, 
      morale_data = morale_data, 
      net_worth_data = net_worth_data, 
      success = food_success and morale_success and net_worth_success
   }

   return summation
end

function Immigration:_find_requirments_by_type_and_pop(available, type, num_citizens)
   local equation = self._sv.immigration_data.growth_requirements[type]
   local equation = string.gsub(equation, 'num_citizens', num_citizens)
   local target = self:_evaluate_equation(equation)
   local label = self._sv.immigration_data[type .. '_label']

   local data = {
      label = label,
      available = available, 
      target = target
   }
   local success = available >= target
   return success, data
end

function Immigration:_evaluate_equation(equation)
   local fn = loadstring('return ' .. equation)
   return fn()
end

function Immigration:_compose_message(job)
   local message_index = rng:get_int(1, #self._sv.immigration_data.messages)
   local message = self._sv.immigration_data.messages[message_index]
   local outcome_statement = self._sv.immigration_data.outcome_statement
   local job_name = radiant.resources.load_json(job).name
   local town_name = stonehearth.town:get_town(self._sv.player_id):get_town_name()


   outcome_statement = string.gsub(outcome_statement, '__job__', job_name)
   outcome_statement = string.gsub(outcome_statement, '__town_name__', town_name)
   return message .. ' ' .. outcome_statement
end



function Immigration:place_citizen(citizen)
   local spawn_point = stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity(citizen, 80)
   local town = stonehearth.town:get_town(self._sv.player_id)

   if not spawn_point then
      --Spawn somewhere near the center of town
      local player_id = town:get_player_id()
      local explored_region = stonehearth.terrain:get_visible_region(player_id):get()
      local centroid = _radiant.csg.get_region_centroid(explored_region):to_closest_int()
      local proposed_location = radiant.terrain.get_point_on_terrain(Point3(centroid.x, 0, centroid.y))
      spawn_point = radiant.terrain.find_placement_point(proposed_location, 20, 30)
   end

   radiant.terrain.place_entity(citizen, spawn_point)

   --Give the entity the task to run to the banner
   self._approach_task = citizen:get_component('stonehearth:ai')
                           :get_task_group('stonehearth:unit_control')
                                 :create_task('stonehearth:goto_town_center', {town = town})
                                 :set_priority(stonehearth.constants.priorities.unit_control.DEFAULT)
                                 :once()
                                 :start()

   self:_inform_player(citizen)

   --TODO: attach particle effect
end

function Immigration:_inform_player(citizen)
   --Send another bulletin with the dude's name, etc
   local town_name = stonehearth.town:get_town(self._sv.player_id):get_town_name()
   local citizen_name = radiant.entities.get_name(citizen)
   local title = self._sv.immigration_data.success_title
   title = string.gsub(title, '__name__', citizen_name)
   title = string.gsub(title, '__town_name__', town_name)
   local pop = stonehearth.population:get_population(self._sv.player_id)
   pop:show_notification_for_citizen(citizen, title)
end

--- Only actually spawn the object after the user clicks OK
function Immigration:_on_accepted()
   local pop = stonehearth.population:get_population(self._sv.player_id)
   local citizen = pop:create_new_citizen()

   citizen:add_component('stonehearth:job')
               :promote_to('stonehearth:jobs:worker')

   self:place_citizen(citizen)
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function Immigration:_on_declined()
   if self._sv.timer then
      self._sv.timer:destroy()
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function Immigration:_create_timer(duration)
   self._sv.timer = stonehearth.calendar:set_timer(duration, function() 
      self:_timer_callback()
   end)
end

function Immigration:_timer_callback()
   if self._sv.immigration_bulletin then
      local bulletin_id = self._sv.immigration_bulletin:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      self:_stop_timer()
   end
end

function Immigration:_stop_timer()
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end
end


return Immigration