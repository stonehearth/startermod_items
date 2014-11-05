local build_util = require 'stonehearth.lib.build_util'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local lrbt_util = {}

local WOODEN_COLUMN = 'stonehearth:wooden_column'
local WOODEN_WALL = 'stonehearth:wooden_wall'
local WOODEN_FLOOR = 'stonehearth:entities:wooden_floor'
local WOODEN_FLOOR_PATTERN = '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb'
local WOODEN_ROOF = 'stonehearth:wooden_peaked_roof'

function lrbt_util.create_workers(autotest, x, y)
   local workers = {}

   local function create_worker(x, y)
      local worker = autotest.env:create_person(x, y, { job = 'worker' })
      workers[worker:get_id()] = worker
      
      local timer
      timer = stonehearth.calendar:set_interval('1m', function()
            if not worker:is_valid() then
               timer:destroy()
               return
            end
            worker:get_component('stonehearth:attributes')
                     :set_attribute('hunger', 1)
                     :set_attribute('sleepiness', 0)
         end)
   end

   create_worker(x, y)
   create_worker(x, y + 2)
   create_worker(x + 2, y)
   create_worker(x + 2, y + 2)

   return workers
end

function lrbt_util.fund_construction(autotest, buildings)
   local x, y = 18, 20
   
   for _, building in pairs(buildings) do
      local cost = build_util.get_cost(building)
      for material, _ in pairs(cost.resources) do
         if material:find('wood') then
            x = x - 2
            lrbt_util.create_endless_entity(autotest, x, y, 2, 2, 'stonehearth:oak_log')
         end
      end
      local stockpile_x = x
      for uri, _ in pairs(cost.items) do
         x = x - 2
         lrbt_util.create_endless_entity(autotest, x, y, 2, 2, uri)
      end
      local stockpile_w = stockpile_x - x
      autotest.env:create_stockpile(x, y, { size = { x = stockpile_w, y = 2 }})
   end
   
   lrbt_util.create_workers(autotest, x - 1, y)
end

function lrbt_util.get_area(structure)
   return structure:get_component('destination'):get_region():get():get_area()
end

function lrbt_util.count_structures(building)
   local counts = {}
   local structures = building:get_component('stonehearth:building')
                                 :get_all_structures()
   for type, entries in pairs(structures) do
      if next(entries) then
         counts[type] = 0
         for _, entry in pairs(entries) do
            counts[type] = counts[type] + 1
         end
      end
   end
   return counts
end

function lrbt_util.create_wooden_floor(session, cube)
   return stonehearth.build:add_floor(session, WOODEN_FLOOR, cube, WOODEN_FLOOR_PATTERN)
end

function lrbt_util.erase_floor(session, cube)
   stonehearth.build:erase_floor(session, cube)
end

function lrbt_util.create_wooden_wall(session, p0, p1)
   local t, n = 'x', 'z'
   if p0.x == p1.x then
      t, n = 'z', 'x'
   end
   local normal = Point3(0, 0, 0)
   if p0[t] < p1[t] then
      normal[n] = 1
   else
      p0, p1 = p1, p0
      normal[n] = -1
   end

   return stonehearth.build:add_wall(session, WOODEN_COLUMN, WOODEN_WALL, p0, p1, normal)
end

function lrbt_util.grow_wooden_walls(session, building)
   return stonehearth.build:grow_walls(building, WOODEN_COLUMN, WOODEN_WALL)
end

function lrbt_util.grow_wooden_roof(session, building)
   return stonehearth.build:grow_roof(building, WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right' },
         nine_grid_max_height = 10,
      })
end

function lrbt_util.create_endless_entity(autotest, x0, y0, w, h, uri)
   local function create_another_entity(x, y)
      local trace
      local entity = autotest.env:create_entity(x, y, uri)
      trace = entity:trace('make more logs')
                        :on_destroyed(function()
                              trace:destroy()
                              if not autotest:is_finished() then
                                 create_another_entity(x, y, uri)
                              end
                           end)
   end

   for x = x0,x0+w-1 do 
      for y = y0,y0+h-1 do
         create_another_entity(x, y)
      end
   end                        
end

function lrbt_util.succeed_when_buildings_finished(autotest, buildings, scaffoldings)
   local succeeded = false
   local function succeed_if_finished()
      if not succeeded then
         for _, building in pairs(buildings) do
            if not building:get_component('stonehearth:construction_progress'):get_finished() then
               return
            end
         end
         for _, scaffolding in pairs(scaffoldings) do
            if not scaffolding:get_component('destination'):get_region():get():empty() then
               return
            end
         end
         succeeded = true
         autotest:success()
      end
   end
   
   local function install_listeners(t)
      for _, entity in pairs(t) do
         radiant.events.listen(entity, 'stonehearth:construction:finished_changed', function()
               succeed_if_finished();
            end)
      end
   end
   
   install_listeners(buildings)
   install_listeners(scaffoldings)
end

function lrbt_util.succeed_when_buildings_destroyed(autotest, buildings)
   for _, building in pairs(buildings) do
      radiant.events.listen_once(building, 'radiant:entity:pre_destroy', function()
            buildings[building:get_id()] = nil
            if not next(buildings) then
               autotest:success()
            end
         end)
   end
end


local function track_buildings_and_scaffolding_creation(cb)
   local scaffolding, buildings = {}, {}

   local building_listener = radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         if entity:is_valid() and entity:get_uri() == 'stonehearth:entities:building' then
            buildings[entity:get_id()] = entity
         end
         if entity:is_valid() and entity:get_uri() == 'stonehearth:scaffolding' then
            scaffolding[entity:get_id()] = entity
         end
      end)

   cb()

   building_listener:destroy()
   
   -- the act of running through all the steps may have deleted some of these objects.
   -- remove them from the arrays
   stonehearth.build:clear_undo_stack()
   
   local function prune_dead_entities(t)
      for id, entity in pairs(t) do
         if not entity:is_valid() then
            t[id] = nil
         end
      end
   end
   
   prune_dead_entities(buildings)
   prune_dead_entities(scaffolding)

   return buildings, scaffolding
end

function lrbt_util.create_buildings(autotest, setup_building_fn)
   local buildings, scaffolding = track_buildings_and_scaffolding_creation(function()
         -- now run `cd` to create all the building structures.
         local session = autotest.env:get_player_session()
         local steps

         -- setup may complete designing the entire building.  if not, it will return
         -- a list of functions to run, one for each subsequent step
         stonehearth.build:do_command('setup', nil, function()
               steps = setup_building_fn(autotest, session)
            end)

         if type(steps) == 'table' then
            for i, step_cb in ipairs(steps) do
               stonehearth.build:do_command('step ' .. tostring(i), nil, function()
                     step_cb()
                  end)
            end
         end
      end)

   return buildings, scaffolding
end

function lrbt_util.create_template(autotest, template_name, cb)
   local buildings = lrbt_util.create_buildings(autotest, cb)

   -- template test only works with the first building
   local _, building = next(buildings)
   local centroid = build_util.get_building_centroid(building)
   build_util.save_template(building, { name = template_name }, true)

   -- nuke the buildings
   for _, b in pairs(buildings) do
      radiant.entities.destroy_entity(b)
   end
   
   return centroid
end

function lrbt_util.mark_buildings_active(autotest, buildings)
   for _, building in pairs(buildings) do
      stonehearth.build:set_active(building, true)
   end
end

function lrbt_util.place_template(autotest, template_name, location, centroid, rotation)
   -- drop the template
   local buildings, scaffolding = track_buildings_and_scaffolding_creation(function()
         local session = autotest.env:get_player_session()
         stonehearth.build:do_command('place template', nil, function()
               stonehearth.build:build_template(session, template_name, location, centroid, rotation or 0)
            end)
      end)

   return buildings, scaffolding
end

return lrbt_util
