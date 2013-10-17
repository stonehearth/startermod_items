radiant.mods.load('stonehearth')

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Calendar = require 'services.calendar.calendar_service'

local FirepitComponent = class()

function FirepitComponent:__init(entity, data_store)
   radiant.check.is_entity(entity)
   self._entity = entity

   self._my_wood = nil
   self._light_task = nil
   self._curr_fire_effect = nil
   self._am_lighting_fire = false

   self._seats = nil

   self._data = data_store:get_data()
   self._data.is_lit = false
   self._data_store = data_store
   self._data_store:mark_changed()

   self._time_constants = Calendar.get_constants()

   --When the firepit is added to the world,
   --wait an hour and check if we should light it
   radiant.terrain.on_place(self._entity, function()
      radiant.events.listen('radiant:events:calendar:hourly', self)
      self._placement_time = Calendar.get_time_and_date().hour
      if self._placement_time == 23 then
         self._placement_time = 0
      end
   end)

   --When a firepit is removed from the world, reset its tasks and variables
   radiant.terrain.on_remove(self._entity, function()
      return self:_on_remove()
   end)

   --When the firepit is destroyed, destroy its seat entities too
   --TODO: right now we're destroying the firepit on move. We should just save
   --the original and relocate, in which case we'll need to do cleanup
   --on remove from terrain
   radiant.entities.on_destroy(self._entity, function()
      return self:_on_destroy()
   end)
end

function FirepitComponent:extend(json)
   -- TODO: figure out how to communicate "where wolves won't go"
   if json and json.effective_radius then
      self._effective_raidus = json.effective_radius
   end
end

--- HACK: An hour after placing the firepit, initate all firepit related logic
-- If you put a firepit down with place_on_terrain and *immediately* run to get wood for it,
-- you get a weird LUA error. Waiting a little while in-game seems to help.
-- This is not a final solution! We need to look deeper into this problem.
-- What about other computers, processors, etc, for example?
-- NOTE: Doesn't seem to be the data binding, etc. as commenting that out produces the same error
FirepitComponent['radiant:events:calendar:hourly'] = function(self, calendar)
   if calendar.hour > self._placement_time + 1 then
      self:_should_light_fire()

      radiant.events.listen('radiant:events:calendar:sunset', self)
      radiant.events.listen('radiant:events:calendar:sunrise', self)

      radiant.events.unlisten('radiant:events:calendar:hourly', self)
   end
end

--- On destroy, nuke the events, seats, and task/pathfinders
function FirepitComponent:_on_destroy()
   radiant.events.unlisten('radiant:events:calendar:sunrise', self)
   radiant.events.unlisten('radiant:events:calendar:sunset', self)

   if self._seats then
      for i, v in ipairs(self._seats) do
         radiant.entities.destroy_entity(v)
      end
   end
   self._seats = nil

   if self._light_task then
      self._light_task:stop()
      self._light_task:destroy()
   end
end

--- Disable a bunch of things on remove
-- Note: this works but is unused at present (as adding back a removed item)
-- adds it back invisibly. Should I keep this code or wait till we've fixed it?
-- TODO: is this everything? You shouldn't be able to move a lit firepit.
function FirepitComponent:_on_remove()
   radiant.events.unlisten('radiant:events:calendar:sunrise', self)
   radiant.events.unlisten('radiant:events:calendar:sunset', self)

   if self._seats then
      for i, v in ipairs(self._seats) do
         radiant.entities.destroy_entity(v)
      end
   end

   if self._light_task then
      self._light_task:stop()
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
FirepitComponent['radiant:events:calendar:sunset'] = function (self)
   self:_should_light_fire()
end

--- Reused between this and when we check to see if we should
-- light the fire after place.
function FirepitComponent:_should_light_fire()
   --Only light fires after dark
   local curr_time = Calendar.get_time_and_date()
   if curr_time.hour >= self._time_constants.event_times.sunset or
      curr_time.hour < self._time_constants.event_times.sunrise then

      --Only do this if we haven't yet started the process for tonight
      if self._am_lighting_fire then
         return
      end

      self._am_lighting_fire = true

      self:extinguish()
      if not self._seats then
         self:_add_seats()
      end
      self:_init_gather_wood_task()
   end
end

--- At sunrise, the fire eats the log inside of it
FirepitComponent['radiant:events:calendar:sunrise'] = function (self)
   if self._light_task then
      self._light_task:stop()
   end
   self:extinguish()
   self._am_lighting_fire = false
   self._entity:get_component('stonehearth:placed_item'):set_usage(false)
end

---Adds 8 seats around the firepit
--TODO: add a random element to the placement of the seats.
function FirepitComponent:_add_seats()
   --Is this being called for an entity that doesn't exist?
   if not self then
      return
   end
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
end

--- Create a worker task to gather wood
function FirepitComponent:_init_gather_wood_task()
   if not self._light_task then
      local ws = radiant.mods.load('stonehearth').worker_scheduler
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
         local item = item_entity:get_component('item')
         if not item then
            return false
         end
         --TODO: if we add "wood" as a property to proxy items made of wood, we could be in trouble...
         return item:get_material() == "wood"
      end

      --Create the pickup task
      self._light_task = self._worker_scheduler:add_worker_task('lighting_fire_task')
                  :set_worker_filter_fn(not_carrying_fn)
                  :set_work_object_filter_fn(object_filter_fn)

      self._light_task:set_action_fn(
         function (path)
            -- TODO: destroying the worker task is not necessarily destroying the pf.
            if self then
               self._entity:get_component('stonehearth:placed_item'):set_usage(true)
               return 'stonehearth:light_fire', path, self._entity, self._light_task
            else
               return
            end
         end
      )
   end
   self._light_task:start()
end

--- Returns whether or not the firepit is lit
function FirepitComponent:is_lit()
   return self._data.is_lit
end

--- Adds wood to the fire
-- Create a new entity instead of re-using the old one because if we wanted to do
-- that, we'd have to reparent the log to the fireplace.
-- @param log_entity to add to the fire
function FirepitComponent:light(log)
   self._my_wood = log
   self._curr_fire_effect =
      radiant.effects.run_effect(self._entity, '/stonehearth/data/effects/firepit_effect')
   self._data.is_lit = true
   self._data_store:mark_changed()
end

--- If there is wood, destroy it and extinguish the particles
function FirepitComponent:extinguish()
   if self._my_wood then
      radiant.entities.remove_child(self._entity, self._my_wood)
      radiant.entities.destroy_entity(self._my_wood)
      self._my_wood = nil
   end

   if self._curr_fire_effect then
      self._curr_fire_effect:stop()
   end
   self._data.is_lit = false
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
