local build_util = require 'stonehearth.lib.build_util'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local lrbt_util = {}

local WOODEN_WALL    = stonehearth.constants.construction.DEFAULT_WOOD_WALL_BRUSH
local WOODEN_ROOF    = stonehearth.constants.construction.DEFAULT_WOOD_ROOF_BRUSH
local WOODEN_FLOOR   = stonehearth.constants.construction.DEFAULT_WOOD_FLOOR_BRUSH
local WOODEN_COLUMN  = stonehearth.constants.construction.DEFAULT_WOOD_COLUMN_BRUSH
local STONE_FLOOR    = stonehearth.constants.construction.DEFAULT_STONE_FLOOR_BRUSH

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
   create_worker(x + 4, y)
   create_worker(x + 4, y + 2)
   create_worker(x + 6, y)
   create_worker(x + 6, y + 2)

   return workers
end

function lrbt_util.fund_construction(autotest, buildings)
   local x, y = 18, 20
   
   lrbt_util.create_endless_entity(autotest, x, y, 3, 3, 'stonehearth:resources:wood:oak_log')
   x = x - 3
   lrbt_util.create_endless_entity(autotest, x, y, 3, 3, 'stonehearth:resources:stone:hunk_of_stone')
   
   for _, building in pairs(buildings) do
      local cost = build_util.get_cost(building)
      local stockpile_x = x
      for uri, _ in pairs(cost.items) do
         x = x - 3
         lrbt_util.create_endless_entity(autotest, x, y, 3, 3, uri)
      end
      local stockpile_w = stockpile_x - x
      autotest.env:create_stockpile(x, y, { size = { x = stockpile_w, y = 3 }})
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
   return stonehearth.build:add_floor(session, WOODEN_FLOOR, cube)
end

function lrbt_util.create_stone_floor(session, cube)
   return stonehearth.build:add_floor(session, STONE_FLOOR, cube)
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

function lrbt_util.grow_wooden_walls(session, entity)
   local _, floor
   if build_util.is_building(entity) then
      local floors = entity:get_component('stonehearth:building')
                              :get_floors()
      _, floor = next(floors)
   else
      floor = entity
   end
   return stonehearth.build:grow_walls(floor, WOODEN_COLUMN, WOODEN_WALL)
end

function lrbt_util.grow_wooden_roof(session, arg1)
   local root_wall
   if arg1:get_component('stonehearth:wall') then
      root_wall = arg1
   elseif arg1:get_component('stonehearth:building') then
      local structures = arg1:get_component('stonehearth:building')
                                 :get_all_structures()
      local _, entry = next(structures['stonehearth:wall'])
      root_wall = entry.entity
   end

   return stonehearth.build:grow_roof(root_wall, {
         brush = WOODEN_ROOF,
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

function lrbt_util.succeed_when_buildings_finished(autotest, buildings)
   local unfinished = {}
   local listeners = {}
   local traces = {}

   local function check_success()
      if radiant.empty(unfinished) then
         for _, listener in pairs(listeners) do
            listener:destroy()
         end
         for _, trace in pairs(traces) do
            trace:destroy()
         end
         autotest:success()
      end
   end

   local create_listener = radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         local id = entity:get_id()
         if entity:get_uri() == 'stonehearth:build:prototypes:scaffolding' then
            local trace = entity:get_component('region_collision_shape')
                     :trace_region('checking success')
                        :on_changed(function(rgn)
                              local finished = rgn and rgn:empty()
                              if finished then
                                 unfinished[id] = nil
                                 check_success()
                              else
                                 unfinished[id] = entity
                              end
                           end)
            traces[id] = trace
            trace:push_object_state()
         end
      end)
   local destroy_listener = radiant.events.listen(radiant, 'radiant:entity:pre_destroy', function(e)
         local entity = e.entity
         unfinished[entity:get_id()] = nil
         local entity = e.entity
         if entity:get_uri() == 'stonehearth:build:prototypes:scaffolding' then
            unfinished[entity:get_id()] = entity
         end
      end)
   table.insert(listeners, create_listener)
   table.insert(listeners, destroy_listener)

   for id, building in pairs(buildings) do
      unfinished[id] = building
      local building_listener = radiant.events.listen(building, 'stonehearth:construction:finished_changed', function()
            local finished = building:get_component('stonehearth:construction_progress'):get_finished()
            if not finished then
               unfinished[id] = building
               return
            end
            unfinished[id] = nil
            check_success()
         end)
      table.insert(listeners, building_listener)
   end
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
         if entity:is_valid() and entity:get_uri() == 'stonehearth:build:prototypes:building' then
            buildings[entity:get_id()] = entity
         end
         if entity:is_valid() and entity:get_uri() == 'stonehearth:build:prototypes:scaffolding' then
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
