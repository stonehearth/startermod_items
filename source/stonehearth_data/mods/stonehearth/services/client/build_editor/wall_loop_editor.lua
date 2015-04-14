local constants = require('constants').construction
local StructureEditor = require 'services.client.build_editor.structure_editor'
local WallLoopEditor = class(StructureEditor)

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

function WallLoopEditor:__init(build_service)
   self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
end

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
            local proxy_fabricator = current_column_editor:get_proxy_fabricator()

            if result.entity == proxy_fabricator then
               return stonehearth.selection.FILTER_IGNORE
            end

            if result.entity:get_component('stonehearth:building') then
               return stonehearth.selection.FILTER_IGNORE
            end

            local location = result.brick
            local rcs = proxy_fabricator:add_component('region_collision_shape')
            local collision_region = rcs:get_region():get():translated(location)

            -- make sure the column is not blocked
            local overlapping_entities = radiant.terrain.get_entities_in_region(collision_region)
            for _, entity in pairs(overlapping_entities) do
               if radiant.entities.is_solid_entity(entity) then
                  return stonehearth.selection.FILTER_IGNORE
               end
            end

            -- make sure point below is supported
            -- can happen when stabbing the side of a block
            if not radiant.terrain.is_standable(location) then
               return stonehearth.selection.FILTER_IGNORE
            end
            
            return result.entity:get_component('terrain') ~= nil
         end)
      :progress(function(selector, location, rotation)
            if location then
               if last_location then
                  location = self:_fit_point_to_constraints(last_location, location, current_column_editor:get_proxy_blueprint())
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
   table.insert(self._queued_points, {
      p0 = radiant.entities.get_world_grid_location(c0:get_proxy_blueprint()),
      p1 = radiant.entities.get_world_grid_location(c1:get_proxy_blueprint()),
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

function WallLoopEditor:_fit_point_to_constraints(p0, p1, column1)  
   p0, p1 = Point3(p0.x, p0.y, p0.z), Point3(p1.x, p1.y, p1.z)
   
   local t, n, dt, d
   if math.abs(p0.x - p1.x) >  math.abs(p0.z - p1.z) then
      t = 'x'
      n = 'z'
   else
      t = 'z'
      n = 'x'
   end
   if p0[t] > p1[t] then
      dt = 1
      d  = math.max(p1[t] - p0[t], -constants.MAX_WALL_SPAN)
   else
      dt = -1
      d  = math.min(p1[t] - p0[t], constants.MAX_WALL_SPAN)
   end
   p1[t] = p0[t] + d
   p1[n] = p0[n]
   p1.y = p0.y
   
   local region = column1:get_component('destination'):get_region():get()
   while not _radiant.client.is_valid_standing_region(region:translated(p1)) and p1[t] ~= p0[t] do
      p1[t] = p1[t] - dt
   end
   return p1
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
