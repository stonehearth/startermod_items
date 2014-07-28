local Vec3 = _radiant.csg.Point3f

local AnalyticsService = class()
local population_service = stonehearth.population

function AnalyticsService:__init(datastore)
   local one_minute = 1000 * 60
   local minutes_elapsed = 0

   -- grr.. radiant.set_realtime_interval() would be nice, so we don't have to
   -- keep rescheduling the timer!!
   local function timer_cb()
      minutes_elapsed = minutes_elapsed + 1
      self:on_minute_poll()
      if minutes_elapsed % 10 == 0 then
         self:on_ten_minute_poll()
      end
      radiant.set_realtime_timer(one_minute, timer_cb)
   end

   radiant.set_realtime_timer(one_minute, timer_cb)
end

function AnalyticsService:initialize()
end

function AnalyticsService:restore(saved_variables)
end

function AnalyticsService:on_minute_poll()
   _radiant.analytics.DesignEvent('game:is_running'):send_event()
end

function AnalyticsService:on_ten_minute_poll()
   local send_string = ""

   -- Send some data for each faction
   -- xxx: this is slightly confused.  don't we want to do it for all players?  when this
   -- code was originally written, the population service tracked pops by faction (not player)
   local populations = population_service:get_all_populations()
   for player_id, pop in pairs(populations) do
      --Send data about # workers
      local citizens = pop:get_citizens()
      local citizen_count = 0
      for _ in pairs(citizens) do
         citizen_count = citizen_count + 1
      end
      send_string = send_string .. "_Num_citizens_" .. citizen_count

      --Send data about resources
      --[[
      local resource_tracker = object_tracker:get_resource_tracker(player_id)
      local items = resource_tracker:get_tracked_items()

      send_string = send_string .. "_Resources_"
      for uri, data in pairs(items) do
         send_string = send_string .. data.name .. "_" .. data.count
      end 
      ]]  
   end

   _radiant.analytics.DesignEvent('game:user_stats'):set_area(send_string):send_event()
end

--- Call the native design event function after doing some common manipulations
-- This is a helper function to speed up the creation of events from entities. 
-- @param event_name : usually in the format of game:activity
-- @param subject : the actor in the thing we're recording. An entity. Optional.
-- @param object : the object in the thing we're recording. An entity. Optional.
-- @param area : a string representing whatever extra data we'd like to convey
-- @param value : a float representing some data about the event
function AnalyticsService:send_design_event(event_name, subject, object, area, value)
   local full_event_name = event_name
   local position = nil

   if subject then
      full_event_name = full_event_name .. ':' ..  self:_format(subject:get_uri())
   end
   if object then
      full_event_name = full_event_name .. ':' ..  self:_format(object:get_uri())

      if object:get_component('mob') then
         position = object:get_component('mob'):get_world_grid_location()
      end
   end

   local design_event = _radiant.analytics.DesignEvent(full_event_name)

   if position then 
      design_event:set_position(Vec3(position.x, position.y, position.z))
   end

   if area then
      design_event:set_area(area)
   end

   if value then
      design_event:set_value(value)
   end

   design_event:send_event()
end

--- Remove : and replace with _
--  TODO: remove stonehearth/mod names also?
function AnalyticsService:_format(input)
   return string.gsub(input, '%W', '_')
end

return AnalyticsService
