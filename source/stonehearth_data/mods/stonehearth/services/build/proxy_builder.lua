local Constants = require 'services.build.constants'
local ProxyColumn = require 'services.build.proxy_column'
local ProxyWall = require 'services.build.proxy_wall'
local ProxyBuilder = require 'services.build.proxy_builder'
local ProxyBuilder = class()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyBuilder:__init(derived, on_mouse, on_keyboard)
   self._derived = derived
   self._on_mouse = on_mouse
   self._on_keyboard = on_keyboard
   
   self._container_entity = radiant.entities.create_entity()
   self._container_render_entity = _radiant.client.create_render_entity(1, self._container_entity)
end

function ProxyBuilder:set_wall_uri(wall_uri)
   self._wall_uri = wall_uri
   return self._derived
end

function ProxyBuilder:set_column_uri(column_uri)
   self._column_uri = column_uri
   return self._derived
end

function ProxyBuilder:_clear()
   if self._columns then
      for i, column in ipairs(self._columns) do
         column:destroy()
      end
      self._columns = nil
   end
   
   if self._walls then
      for i, wall in ipairs(self._walls) do
         wall:destroy()
      end
      self._walls = nil
   end
   
   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end
end

function ProxyBuilder:_start()
   assert(not self._columns)
   assert(not self._walls)
   self._columns = {}
   self._walls = {}
   
   self._input_capture = _radiant.client.capture_input()
   self._input_capture:on_input(function(e)
      if e.type == _radiant.client.Input.MOUSE then
         return self._on_mouse(self._derived, e.mouse)
      elseif e.type == _radiant.client.Input.KEYBOARD then
         return self._on_keyboard(self._derived, e.mouse)
      end
      return false
   end)   
end

function ProxyBuilder:move_to(location)
   self._container_entity:add_component('mob'):set_location_grid_aligned(location)
end

function ProxyBuilder:get_column(i)
   if i > 0 then
      return self._columns[i]
   end
   return self._columns[#self._columns + i + 1]
end

function ProxyBuilder:get_column_count()
   return #self._columns
end

function ProxyBuilder:add_column()
   local column = ProxyColumn(self._container_entity, self._column_uri)
   table.insert(self._columns, column)
   return column
end

function ProxyBuilder:add_wall()
   local wall = ProxyWall(self._container_entity, self._wall_uri)
   table.insert(self._walls, wall)
   return wall
end

function ProxyBuilder:shift_down()
  return _radiant.client.is_key_down(_radiant.client.KeyboardInput.LEFT_SHIFT) or
         _radiant.client.is_key_down(_radiant.client.KeyboardInput.RIGHT_SHIFT)
end

function ProxyBuilder:_package_entity(fabrication)
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
         mob = entity:get_component_data('mob'),
         destination = entity:get_component_data('destination'),
         [component_name] = data
      }
   };
   return result
end

function ProxyBuilder:publish()
   local structures = {}
   for _, wall in ipairs(self._walls) do
      table.insert(structures, self:_package_entity(wall))
   end
   for _, column in ipairs(self._columns) do
      table.insert(structures, self:_package_entity(column))
   end
   _radiant.call('stonehearth:build_structures', structures);
   self:_clear()
end

return ProxyBuilder
