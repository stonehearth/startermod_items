local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

radiant.mods.load('stonehearth')

local FirepitComponent = class()

function FirepitComponent:__init(entity, data_binding)
   radiant.check.is_entity(entity)
   self._entity = entity

   self._is_lit = false
   self._my_wood = nil
   self._light_task = nil

   self._seats = nil

   radiant.events.listen('radiant:events:calendar:sunrise', self)
   radiant.events.listen('radiant:events:calendar:sunset', self)
end

function FirepitComponent:extend(json)
   -- not really...
   if json and json.effective_radius then
      self._effective_raidus = json.effective_radius
   end
end

---Adds 8 seats around the firepit
--TODO: actually have a place for people to sit down/lie down
--TODO: add a random element to the placement of the seats.
function FirepitComponent:_add_seats()
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
   local seat = radiant.entities.create_entity('stonehearth:fire_pit_seat')
   local seat_comp = seat:get_component('stonehearth:center_of_attention_spot')
   seat_comp:add_to_center_of_attention(self._entity, seat_number)
   self._seats[seat_number] = seat
   radiant.terrain.place_entity(seat, location)
end

--- At sunset, start firepit activities
-- Tell a worker to fill the fire with wood, and light it
-- If there aren't seats around the fire yet, create them
-- If there's already wood in the fire from the previous day, it goes out now.
-- TODO: Maybe start this earlier? People should put wood on the fire before the wolves come out.
-- TODO: alternatively, find a way to make this REALLY IMPORTANT relative to other worker tasks
FirepitComponent['radiant:events:calendar:sunset'] = function (self)
   self:extinguish()
   if not self._seats then
      self:_add_seats()
   end
   self:_init_gather_wood_task()
end

--- At sunrise, the fire eats the log inside of it
FirepitComponent['radiant:events:calendar:sunrise'] = function (self)
   if self._light_task then
      self._light_task:stop()
   end
   self:extinguish()
end

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
            return 'stonehearth:light_fire', path, self._entity, self._light_task
         end
      )
   end
   self._light_task:start()
end

function FirepitComponent:is_lit()
   return self._is_lit
end

--- Adds wood to the fire
-- Create a new entity instead of re-using the old one because if we wanted to do
-- that, we'd have to reparent the log to the fireplace.
-- @param log_entity to add to the fire
function FirepitComponent:light(log)
   self._my_wood = log
   self._curr_fire_effect =
      radiant.effects.run_effect(self._entity, '/stonehearth/data/effects/firepit_effect')
   self._is_lit = true
end

--- if there is wood, destroy it and extinguish the particles
function FirepitComponent:extinguish()
   if self._my_wood then
      radiant.entities.remove_child(self._entity, self._my_wood)
      radiant.entities.destroy_entity(self._my_wood)
      self._my_wood = nil
   end

   if self._curr_fire_effect then
      self._curr_fire_effect:stop()
   end
   self._is_lit = false
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
