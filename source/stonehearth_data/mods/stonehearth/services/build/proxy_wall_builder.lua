local Constants = require 'services.build.constants'
local ProxyColumn = require 'services.build.proxy_column'
local ProxyWall = require 'services.build.proxy_wall'
local ProxyWallBuilder = class()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyWallBuilder:__init()
end

function ProxyWallBuilder:set_wall_uri(wall_uri)
   self._wall_uri = wall_uri
   return self
end
function ProxyWallBuilder:set_column_uri(column_uri)
   self._column_uri = column_uri
   return self
end

function ProxyWallBuilder:build_chain_of_walls()
   self:_start_chain()

   self._input_capture = _radiant.client.capture_input()
   self._input_capture:on_input(function(e)
      if e.type == _radiant.client.Input.MOUSE then
         return self:_on_mouse_event(e.mouse)
      elseif e.type == _radiant.client.Input.KEYBOARD then
         return self:_on_keyboard_event(e.keyboard)
      end
      return false
   end)

   return self
end

function ProxyWallBuilder:_start_chain()
   assert(not self._columns)
   assert(not self._walls)
   self._columns = {}
   self._walls = {}
   self:_add_column()
end

function ProxyWallBuilder:_add_column()
   local column = ProxyColumn(self._column_uri)
   table.insert(self._columns, column)
   return column
end

function ProxyWallBuilder:_get_last_column()
   return self._columns[#self._columns]
end

function ProxyWallBuilder:_get_last_segment()
   local len = #self._columns
   return self._columns[len - 1], self._columns[len]
end

function ProxyWallBuilder:_fit_point_to_constraints(pt)
   local column_a, column_b = self:_get_last_segment()
   if not column_a then
      return pt
   end
   -- constrain the 2nd column to be no more than the max wall
   -- distance away from the first
   local pt_a = column_a:get_location()
   local pt_b = pt

   local t, n, dt, d
   if math.abs(pt_a.x - pt_b.x) >  math.abs(pt_a.z - pt_b.z) then
      t = 'x'
      n = 'z'
   else
      t = 'z'
      n = 'x'
   end
   if pt_a[t] > pt_b[t] then
      dt = 1
      d  = math.max(pt_b[t] - pt_a[t], -Constants.MAX_WALL_SPAN)
   else
      dt = -1
      d  = math.min(pt_b[t] - pt_a[t], Constants.MAX_WALL_SPAN)
   end
   pt_b[t] = pt_a[t] + d
   pt_b[n] = pt_a[n]
   pt_b.y = pt_a.y
   
   local region = column_b:get_region():get()
   while not _radiant.client.is_valid_standing_region(region:translated(pt_b)) and pt_b[t] ~= pt_a[t] do
      pt_b[t] = pt_b[t] - dt
   end
   return pt_b
end

function ProxyWallBuilder:_add_new_segment()
   local column_a, column_b = self:_get_last_segment()
   if column_a and column_b then
      local wall = ProxyWall(self._wall_uri)
      table.insert(self._walls, wall)
      wall:connect_to(column_a, column_b)
      return wall
   end
end

function ProxyWallBuilder:_shift_down()
  return _radiant.client.is_key_down(_radiant.client.KeyboardInput.LEFT_SHIFT) or
         _radiant.client.is_key_down(_radiant.client.KeyboardInput.RIGHT_SHIFT)
end

function ProxyWallBuilder:_on_mouse_event(e)
   local query = _radiant.client.query_scene(e.x, e.y)
   if query.location then
      local location = self:_fit_point_to_constraints(query.location + query.normal)
      self:_get_last_column():move_to(location)      
      if e:up(1) then
         self:_add_new_segment()      
         if self:_shift_down() or #self._columns < 2 then
            self:_add_column():move_to(location)
         else
            self:publish()
            self._input_capture:destroy()
            self._input_capture = nil
         end
         return true
      end
   end
   return false
end

function ProxyWallBuilder:_package_entity(fabrication)
   local entity = fabrication:get_entity()
   local component_name = fabrication:get_component_name()
   local data = entity:get_component_data(component_name)
   
-- remove the paint_mode.  we just set it here so the blueprint
   -- would be rendered differently
   data.paint_mode = nil
   
   local md = entity:get_component_data('mob')
   local result = {
      entity = entity:get_uri(),
      components = {
         mob = md,
         [component_name] = data
      }
   };
   return result
end

function ProxyWallBuilder:publish()
   local structures = {}
   for _, column in ipairs(self._columns) do
      table.insert(structures, self:_package_entity(column))
   end
   for _, wall in ipairs(self._walls) do
      --table.insert(structures, self:_package_entity(wall))
   end
   _radiant.call('stonehearth:build_structures', structures);
end

function ProxyWallBuilder:_on_keyboard_event(e)
   return false
end

function ProxyWallBuilder:place_new_wall_xxx(session, response)
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   local cursor_entity = radiant.entities.create_entity()
   local mob = cursor_entity:add_component('mob')
   mob:set_interpolate_movement(false)
   
   -- add a render object so the cursor entity gets rendered.
   local cursor_render_entity = _radiant.client.create_render_entity(1, cursor_entity)
   local node = h3dRadiantCreateStockpileNode(cursor_render_entity:get_node(), 'stockpile designation')

   -- change the actual game cursor
   local stockpile_cursor = _radiant.client.set_cursor('stonehearth:cursors:create_stockpile')

   local cleanup = function()
      stockpile_cursor:destroy()
      _radiant.client.destroy_authoring_entity(cursor_entity:get_id())
   end

   _radiant.client.select_xz_region()
      :progress(function (box)
            mob:set_location_grid_aligned(box.min)
            h3dRadiantResizeStockpileNode(node, box.max.x - box.min.x + 1, box.max.z - box.min.z + 1);
         end)
      :done(function (box)
            local size = {
               box.max.x - box.min.x + 1,
               box.max.z - box.min.z + 1,
            }
            _radiant.call('stonehearth:create_stockpile', box.min, size)
                     :done(function(r)
                           response:resolve(r)
                        end)
                     :always(function()
                           cleanup()
                        end)
         end)
      :fail(function()
            cleanup()
         end)
end

return ProxyWallBuilder
