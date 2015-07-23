local constants = require('constants').construction
local csg_lib = require 'lib.csg.csg_lib'
local build_util = require 'lib.build_util'
local StructureEditor = require 'services.client.build_editor.structure_editor'
local WallLoopEditor = class(StructureEditor)

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

function WallLoopEditor:__init(build_service)
   self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
end

-- TODO: consider making a linear region selector class for this
function WallLoopEditor:go(column_brush, wall_brush, response)
   self._column_brush = column_brush
   self._wall_brush = wall_brush
   self._response = response

   self._queued_points = {}
   
   local last_location
   local last_column_editor
   local current_column_editor = self:_create_column_editor(column_brush)

   self._selector = stonehearth.selection:select_location()
      :allow_shift_queuing(true)
      :set_min_locations_count(2)
      :set_filter_fn(function(result)
            local entity = result.entity

            if entity == current_column_editor:get_proxy_fabricator() then
               return stonehearth.selection.FILTER_IGNORE
            end

            if last_column_editor then
               -- prohibit zero length walls
               if entity == last_column_editor:get_proxy_fabricator() then
                  return false
               end
            end

            local fc = entity:get_component('stonehearth:fabricator')
            if fc then
               -- allow building on top of other blueprints
               local top_face = result.normal:to_int().y == 1
               if top_face then
                  return true
               end

               -- allow closing the loop with an existing column
               local blueprint = fc:get_blueprint()
               if blueprint:get_component('stonehearth:column') then
                  return stonehearth.selection.FILTER_IGNORE
               end

               return false
            end

            -- prohibit building in designations
            if radiant.entities.get_entity_data(entity, 'stonehearth:designation') then
               return false
            end

            return stonehearth.selection.find_supported_xz_region_filter(result)
         end)
      :progress(function(selector, location, rotation)
            if location then
               if last_location then
                  location = self:_fit_point_to_constraints(last_location, location, current_column_editor)
               end
               current_column_editor:move_to(location)
            else
               current_column_editor:move_to(Point3(0, -100000, 0))
            end
         end)
      :done(function(selector, location, rotation, finished)
            if last_column_editor then
               self:_queue_wall(last_column_editor, current_column_editor)
            end

            self._finished_queuing = finished
            if not finished then
               last_column_editor = current_column_editor
               last_location = radiant.entities.get_world_grid_location(last_column_editor:get_proxy_blueprint())
               current_column_editor = self:_create_column_editor(column_brush)
               current_column_editor:move_to(last_location)
            else
               current_column_editor:destroy()
               response:resolve({ success = true })
            end
         end)
      :fail(function(selector)
            if last_column_editor then
               last_column_editor:destroy()
            end
            current_column_editor:destroy()
            response:reject('reject')
         end)
      :go()
   
   return self
end

function WallLoopEditor:_queue_wall(c0, c1)
   local p0 = radiant.entities.get_world_grid_location(c0:get_proxy_blueprint())
   local p1 = radiant.entities.get_world_grid_location(c1:get_proxy_blueprint())

   table.insert(self._queued_points, {
      p0 = p0,
      p1 = p1,
      editor = c0,
   })
   self:_pump_queue()
end

function WallLoopEditor:_pump_queue()
   if self._waiting_for_response then
      return
   end   
   local entry = table.remove(self._queued_points)
   if not entry then
      if self._finished_queuing then
         self._response:resolve({ success = true })
      end
      return
   end
   local p0, p1 = entry.p0, entry.p1
   local n = p0.x == p1.x and 'x' or 'z'
   local t = n == 'x' and 'z' or 'x'
   local normal = Point3(0, 0, 0)
   if p0[t] < p1[t] then
      normal[n] = 1
   else
      p0, p1 = p1, p0
      normal[n] = -1
   end

   self._waiting_for_response = true
   _radiant.call_obj(self._build_service, 'add_wall_command', self._column_brush, self._wall_brush, p0, p1, normal)
            :done(function(result)
                  if result.new_selection then
                     stonehearth.selection:select_entity(result.new_selection)
                  end
               end)            
            :fail(function(result)
                  assert(false)
               end)
            :always(function ()
                  entry.editor:destroy()
                  self._waiting_for_response = false
               end)
end

function WallLoopEditor:_fit_point_to_constraints(p0, p1, column_editor)
   if not p0 or not p1 then
      return nil
   end

   local t, n
   if math.abs(p0.x - p1.x) >  math.abs(p0.z - p1.z) then
      t = 'x'
      n = 'z'
   else
      t = 'z'
      n = 'x'
   end

   local d = math.min(math.abs(p1[t] - p0[t]), constants.MAX_WALL_SPAN)
   local dt = p1[t] > p0[t] and 1 or -1

   local start = Point3(p0)
   local current = Point3(p0)
   local max = Point3(p0)
   max[t] = p0[t] + d*dt

   while current ~= max do
      local test = Point3(current)
      test[t] = test[t] + dt
      if not self:_is_valid_location(test, column_editor) then
         break
      end
      current = test
   end

   return current
end

function WallLoopEditor:_is_valid_location(location, column_editor)
   local blueprint = column_editor:get_proxy_blueprint()
   local region = blueprint:add_component('destination'):get_region():get():translated(location)

   local entities = radiant.terrain.get_entities_in_region(region)
   local valid = true

   -- check if we're obstructed by any illegal entities
   for _, entity in pairs(entities) do
      -- check if collision region is clear
      if radiant.entities.is_solid_entity(entity) then
         valid = false
         break
      end

      -- don't allow other blueprints inside our region
      local fabricator = entity:get_component('stonehearth:fabricator')
      if fabricator then
         local blueprint = fabricator:get_blueprint()
         if not blueprint:get_component('stonehearth:column') then
            valid = false
            break
         end
      end

      -- don't allow designations inside our region
      if radiant.entities.get_entity_data(entity, 'stonehearth:designation') then
         valid = false
         break
      end
   end

   if not valid then
      return false
   end

   -- check if the footprint is supported by terrain or another building/blueprint
   local support_region = csg_lib.get_region_footprint(region)
   support_region:translate(-Point3.unit_y)
   local entities = radiant.terrain.get_entities_in_region(support_region)
   valid = false

   for _, entity in pairs(entities) do
      -- terrain is valid support
      if entity:get_id() == 1 then
         valid = true
         break
      end

      -- buildings are valid support
      local construction_data = entity:get_component('stonehearth:construction_data')
      if construction_data then
         valid = true
         break
      end

      -- blueprints are valid support
      local fabricator = entity:get_component('stonehearth:fabricator')
      if fabricator then
         valid = true
         break
      end
   end

   return valid
end

function WallLoopEditor:_create_column_editor(column_brush)
   local column_editor = StructureEditor()
   local column = column_editor:create_blueprint('stonehearth:build:prototypes:column', 'stonehearth:column')
   
   column:get_component('stonehearth:column')
            :set_brush(column_brush)
            :layout()

   return column_editor
end

return WallLoopEditor
