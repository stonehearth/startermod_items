local priorities = require('constants').priorities.worker_task

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

   --Listen on terrain for when this entity is added/removed
   local added_cb = function(id, entity)
      self:_on_entity_add(id, entity)
   end
   local removed_cb = function(id)
      self:_on_entity_remove(id)
   end
   self._promise = radiant.terrain.trace_world_entities('firepit self-tracker',
      added_cb, removed_cb)
end

function FirepitComponent:destroy()
   --When the firepit is destroyed, destroy its seat entities too
   self:_shutdown()
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
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
      self:_startup()
   end
end

function FirepitComponent:_on_entity_remove(id)
   if self._entity and self._entity:get_id() == id then
      self:_shutdown()
   end
end

function FirepitComponent:_startup()
   log:debug('listining for calendar sunset/sunrise events')
   radiant.events.listen(stonehearth.calendar, 'stonehearth:sunrise', self, self._start_or_stop_firepit)
   radiant.events.listen(stonehearth.calendar, 'stonehearth:sunset', self, self._start_or_stop_firepit)
   self:_start_or_stop_firepit()
end

--- Stop listening for events, destroy seats, terminate effects and tasks
-- Call when firepit is moving or being destroyed.
function FirepitComponent:_shutdown()
   log:debug('unlistining for calendar sunset/sunrise events')
   radiant.events.unlisten(stonehearth.calendar, 'stonehearth:sunrise', self, self._start_or_stop_firepit)
   radiant.events.unlisten(stonehearth.calendar, 'stonehearth:sunset', self, self._start_or_stop_firepit)
   self:_extinguish()
   self._am_lighting_fire = false

   if self._seats then
      for i, v in ipairs(self._seats) do
         log:debug('destroying firepit seat %s', tostring(v))
         radiant.entities.destroy_entity(v)
      end
   end
   self._seats = nil
end

--- Returns the firepit's datastore.
-- Useful for people listening in on when the fires are lit.
function FirepitComponent:get_data_store()
   return self._data_store
end

--- Reused between this and when we check to see if we should
-- light the fire after place.
function FirepitComponent:_start_or_stop_firepit()
   --Make sure we're not calling after the entity has been gc'd.
   if not self then
      return
   end

   --Only light fires after dark
   local time_constants = stonehearth.calendar.get_constants()
   local curr_time = stonehearth.calendar.get_time_and_date()
   local should_light_fire = curr_time.hour >= time_constants.event_times.sunset or
                             curr_time.hour < time_constants.event_times.sunrise

   log:spam('in _start_or_stop_fire (should? %s  is? %s)', tostring(should_light_fire), tostring(self._am_lighting_fire))
   if should_light_fire and not self._am_lighting_fire then
      log:detail('decided to light the fire!')
      self._am_lighting_fire = true
      self:_init_gather_wood_task()
   elseif not should_light_fire and self._am_lighting_fire then      
      log:detail('decided to put out the fire!')
      self._am_lighting_fire = false
      self:_extinguish()
   end
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

   local faction = radiant.entities.get_faction(self._entity)
   local town = stonehearth.town:get_town(faction)

   self._light_task = town:create_worker_task('stonehearth:light_firepit', { firepit = self })
                                   :set_name('light firepit')
                                   :once()
                                   :start()
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
   radiant.events.trigger(self._entity, 'stonehearth:fire:lit', { lit = true })
   self._data_store:mark_changed()
end

--- If there is wood, destroy it and extinguish the particles
function FirepitComponent:_extinguish()
   log:debug('extinguishing the fire')

   local ec = self._entity:add_component('entity_container')
   while ec:num_children() > 0 do
      for id, log in ec:each_child() do
         radiant.entities.destroy_entity(log)
         break
      end
   end

   if self._curr_fire_effect then
      self._curr_fire_effect:stop()
   end
   
   if self._light_task then
      self._light_task:destroy()
      self._light_task = nil
   end

   self._data.is_lit = false
   radiant.events.trigger(self._entity, 'stonehearth:fire:lit', { lit = false })
end

return FirepitComponent
