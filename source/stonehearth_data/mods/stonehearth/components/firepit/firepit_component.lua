local priorities = require('constants').priorities.worker_task
local calendar = stonehearth.calendar

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local log = radiant.log.create_logger('firepit')
local FirepitComponent = class()
FirepitComponent.__classname = 'FirepitComponent'

function FirepitComponent:__init(entity, data_store)
   radiant.check.is_entity(entity)
   self._entity = entity

   self._light_task = nil
   self._curr_fire_effect = nil
   self._am_lighting_fire = false

   self._seats = nil

   self._data = data_store:get_data()
   self._data.is_lit = false
   self._data_store = data_store
   self._data_store:mark_changed()

   self._time_constants = calendar.get_constants()

   --Listen on terrain for when this entity is added/removed
   local added_cb = function(id, entity)
      self:_on_entity_add(id, entity)
   end
   local removed_cb = function(id)
      self:_on_entity_remove(id)
   end
   self._promise = radiant.terrain.trace_world_entities('firepit self-tracker',
      added_cb, removed_cb)

   --When the firepit is destroyed, destroy its seat entities too
   radiant.entities.on_destroy(self._entity, function()
      return self:_stop_functionality()
   end)
end

function FirepitComponent:get_entity()
      return self._entity
end

function FirepitComponent:extend(json)
   -- TODO: replace this with firepit made of fire, wolves won't go near fire
   if json and json.effective_radius then
      self._effective_raidus = json.effective_radius
   end
end

function FirepitComponent:get_fuel_material()
   return 'wood resource'
end

--- If WE are added to the universe, register for events, etc/
function FirepitComponent:_on_entity_add(id, entity)
   if self._entity and self._entity:get_id() == id then
      log:debug('listining for hourly events')
      radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
      self._placement_time = calendar.get_time_and_date().hour
      if self._placement_time == 23 then
         self._placement_time = -1
      end
   end
end

function FirepitComponent:_on_entity_remove(id)
   if self._entity and self._entity:get_id() == id then
      self:_stop_functionality()
   end
end

--- HACK: An hour after placing the firepit, initate all firepit related logic
-- If you put a firepit down with place_on_terrain and *immediately* run to get wood for it,
-- you get a weird LUA error. Waiting a little while in-game seems to help.
-- This is not a final solution! We need to look deeper into this problem.
-- What about other computers, processors, etc, for example?
-- NOTE: Doesn't seem to be the data binding, etc. as commenting that out produces the same error
-- filed as bug: http://bugs.radiant-entertainment.com:8080/browse/SH-22
function FirepitComponent:on_hourly(e)
   if e.now.hour >= self._placement_time + 1 then
      self:_should_light_fire()

      log:debug('switching from hourly to sunset/sunrise events')
      radiant.events.listen(calendar, 'stonehearth:sunrise', self, self.on_sunrise)
      radiant.events.listen(calendar, 'stonehearth:sunset', self, self.on_sunset)

      radiant.events.unlisten(radiant.events, 'stonehearth:hourly', self, self.on_hourly)
   end
end

--- Stop listening for events, destroy seats, terminate effects and tasks
-- Call when firepit is moving or being destroyed.
function FirepitComponent:_stop_functionality()
   log:debug('unlistining for calendar sunset/sunrise events')
   radiant.events.unlisten(radiant.events, 'stonehearth:sunrise', self, self.on_sunrise)
   radiant.events.unlisten(radiant.events, 'stonehearth:sunset', self, self.on_sunset)

   self:extinguish()
   self._am_lighting_fire = false

   if self._seats then
      for i, v in ipairs(self._seats) do
         log:debug('destroying firepit seat %s', tostring(v))
         radiant.entities.destroy_entity(v)
      end
   end
   self._seats = nil

   if self._light_task then
      log:debug('stopping light fire task')
      self._light_task:stop()
      self._light_task:destroy()
   end
end

--- Returns the firepit's datastore.
-- Useful for people listening in on when the fires are lit.
function FirepitComponent:get_data_store()
   return self._data_store
end

--- At sunset, or if the firepit is deployed after sunset, start firepit activities
-- Tell a worker to fill the fire with wood, and light it
-- If there aren't seats around the fire yet, create them
-- If there's already wood in the fire from the previous day, it goes out now.
-- TODO: Find a way to make this REALLY IMPORTANT relative to other worker tasks
function FirepitComponent:on_sunset(e)
   self:_should_light_fire()
end

--- Reused between this and when we check to see if we should
-- light the fire after place.
function FirepitComponent:_should_light_fire()
   --Make sure we're not calling after the entity has been gc'd.
   if not self then
      return
   end

   --Only light fires after dark
   local curr_time = calendar.get_time_and_date()
   if curr_time.hour >= self._time_constants.event_times.sunset or
      curr_time.hour < self._time_constants.event_times.sunrise then

      --Only do this if we haven't yet started the process for tonight
      if self._am_lighting_fire then
         log:spam('time to light the fire, but ignoring (already lighting!)')
         return
      end

      self._am_lighting_fire = true
      log:spam('decided to light the fire!')

      self:extinguish()
      self:_init_gather_wood_task()
   end
end

--- At sunrise, the fire eats the log inside of it
function FirepitComponent:on_sunrise(e)
   if self._light_task then
      log:debug('stopping light fire task on sunrize')
      self._light_task:stop()
      self._light_task = nil
   end
   self:extinguish()
   log:debug('decided it is no longer time to light the fire!')
   self._am_lighting_fire = false
end

---Adds 8 seats around the firepit
--TODO: add a random element to the placement of the seats.
function FirepitComponent:_add_seats()
   log:debug('adding firepit seats')
   self._seats = {}
   local firepit_loc = Point3(radiant.entities.get_world_grid_location(self._entity))
   self:_add_one_seat(1, Point3(firepit_loc.x + 5, firepit_loc.y, firepit_loc.z + 1))
   self:_add_one_seat(2, Point3(firepit_loc.x + -4, firepit_loc.y, firepit_loc.z))
   self:_add_one_seat(3, Point3(firepit_loc.x + 1, firepit_loc.y, firepit_loc.z + 5))
   self:_add_one_seat(4, Point3(firepit_loc.x + 1, firepit_loc.y, firepit_loc.z - 4))
   self:_add_one_seat(5, Point3(firepit_loc.x + 4, firepit_loc.y, firepit_loc.z + 4))
   self:_add_one_seat(6, Point3(firepit_loc.x + 4, firepit_loc.y, firepit_loc.z - 3))
   self:_add_one_seat(7, Point3(firepit_loc.x -3, firepit_loc.y, firepit_loc.z + 3))
   self:_add_one_seat(8, Point3(firepit_loc.x - 2, firepit_loc.y, firepit_loc.z - 3))
end

function FirepitComponent:_add_one_seat(seat_number, location)
   local seat = radiant.entities.create_entity('stonehearth:firepit_seat')
   local seat_comp = seat:get_component('stonehearth:center_of_attention_spot')
   seat_comp:add_to_center_of_attention(self._entity, seat_number)
   self._seats[seat_number] = seat
   radiant.terrain.place_entity(seat, location)
   log:spam('place firepit seat at %s', tostring(location))
end

--- Create a worker task to gather wood
function FirepitComponent:_init_gather_wood_task()
   if self._light_task then
      self._light_task:destroy()
      self._light_task = nil
   end

   self._light_task = stonehearth.tasks:get_scheduler('stonehearth:workers')
                                   :create_task('stonehearth:light_firepit', { firepit = self })
                                   :set_name('light firepit')
                                   :once()
                                   :start()

   --[[   
   self._light_task = 
      local ws = stonehearth.worker_scheduler
      local faction = radiant.entities.get_faction(self._entity)
      assert(faction, "missing a faction on this firepit!")
      self._worker_scheduler = ws:get_worker_scheduler(faction)

      --function filter workers: anyone not carrying anything
      --TODO: figure out how to include people who are already carrying wood
      local not_carrying_fn = function (worker)
         return radiant.entities.get_carrying(worker) == nil
      end

      --Object filter
      local object_filter_fn = function(item_entity)
         -- Why would item_entity ever be nil? Is it not cleared out when it is destroyed?
         if not item_entity then
            return false
         end
         local material = item_entity:get_component('stonehearth:material')
         if not material then
            return false
         end
         return material:is('wood resource')
      end

      log:debug('creating light fire task')

      --Create the pickup task
      self._light_task = self._worker_scheduler:add_worker_task('lighting_fire_task')
                  :set_worker_filter_fn(not_carrying_fn)
                  :set_work_object_filter_fn(object_filter_fn)
                  :set_priority(priorities.LIGHT_FIRE)

      self._light_task:set_action_fn(
         function (path)
            -- TODO: destroying the worker task is not necessarily destroying the pf.
            if self then
               log:debug('dispatching stonehearth:light_fire action')
               return 'stonehearth:light_fire', path, self._entity, self._light_task
            end
         end
      )
   end
   self._light_task:start()
   ]]
end

--- Returns whether or not the firepit is lit
function FirepitComponent:is_lit()
   return self._data.is_lit
end

--- Adds wood to the fire
-- Create a new entity instead of re-using the old one because if we wanted to do
-- that, we'd have to reparent the log to the fireplace.
-- Add the seats now, since we don't want the admire fire pf to start till the fire is lit.
-- @param log_entity to add to the fire
function FirepitComponent:light()
   log:debug('lighting the fire')

   self._curr_fire_effect =
      radiant.effects.run_effect(self._entity, '/stonehearth/data/effects/firepit_effect')
   if not self._seats then
      self:_add_seats()
   end
   self._data.is_lit = true
   radiant.events.trigger(self._entity, 'stonehearth:fire:lit', { lit = true})
   self._data_store:mark_changed()
   self:extinguish()
end

--- If there is wood, destroy it and extinguish the particles
function FirepitComponent:extinguish()
   log:debug('extinguishing the fire')

   local ec = self._entity:get_component('entity_container')
   while ec:num_children() > 0 do
      for id, log in ec:each_child() do
         radiant.entities.destroy_entity(log)
         break
      end
   end

   if self._curr_fire_effect then
      self._curr_fire_effect:stop()
   end
   self._data.is_lit = false
   radiant.events.trigger(self._entity, 'stonehearth:fire:lit', { lit = false})
end

--TODO: do we still need these? In any case, fix spelling

--- In radius of light
-- TODO: determine what radius means, right now it is passed in by the json file
-- @param target_entity The thing we're trying to figure out is in the radius of the firepit
-- @return true if we're in the radius of the light of the fire, false otherwise
function FirepitComponent:in_radius(target_entity)
   local target_location = Point3(radiant.entities.get_world_grid_location(target_entity))
   local world_bounds = self:_get_world_bounds()
   return world_bounds:contains(target_location)
end

function FirepitComponent:_get_world_bounds()
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local bounds = Cube3(Point3(origin.x - self._effective_raidus, origin.y, origin.z - self._effective_raidus),
                        Point3(origin.x + self._effective_raidus, origin.y + 1, origin.z + self._effective_raidus))
   return bounds
end


return FirepitComponent
