local Vec3 = _radiant.csg.Point3f

local AnalyticsService = class()
local population_service = require 'services.population.population_service'
local object_tracker = require 'services.object_tracker.object_tracker_service'

function AnalyticsService:__init()
   radiant.events.listen(radiant.events, 'stonehearth:minute_poll', self, self.on_minute_poll)
   radiant.events.listen(radiant.events, 'stonehearth:ten_minute_poll', self, self.on_ten_minute_poll)
end

function AnalyticsService:on_minute_poll()
   _radiant.analytics.DesignEvent('game:is_running'):send_event()
end

function AnalyticsService:on_ten_minute_poll()
   local send_string = ""

   -- Send some data for each faction
   local factions = population_service:get_all_factions()
   for faction_id, faction in pairs(factions) do

      --Send data about # workers
      -- xxx Fix to show breakdown by class when we can listen on worker class changes; 
      -- Right now, the worker scheduler thinks everyone is a worker since everyone is added a worker
      local citizen_tracker = object_tracker:get_worker_tracker(faction_id)
      local c_data = citizen_tracker:get_data_store():get_data()
      if c_data.entities then
         send_string = send_string .. "_Num_citizens_" .. #c_data.entities
      end

      --Send data about resources
      local resource_tracker = object_tracker:get_resource_tracker(faction_id)
      local r_data = resource_tracker:get_data_store():get_data()

      send_string = send_string .. "_Resources_"
      for uri, data in pairs(r_data.resource_types) do
         send_string = send_string .. data.name .. "_" .. data.count
      end   
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

return AnalyticsService()