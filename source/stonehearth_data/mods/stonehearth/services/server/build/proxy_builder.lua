-- move to the client! -- tony
local ProxyColumn = require 'services.server.build.proxy_column'
local ProxyWall = require 'services.server.build.proxy_wall'
local ProxyPortal = require 'services.server.build.proxy_portal'
local ProxyRoof = require 'services.server.build.proxy_roof'
local ProxyContainer = require 'services.server.build.proxy_container'
local ProxyBuilder = class()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyBuilder:__init(derived, on_mouse, on_keyboard)
   self._derived = derived
   self._on_mouse = on_mouse
   self._on_keyboard = on_keyboard
   self._brushes = {
      wall = 'stonehearth:wooden_wall',
      column = 'stonehearth:wooden_column',
      door = 'stonehearth:wooden_door',
      roof = 'stonehearth:wooden_peaked_roof',
   }
   
   self._rotation = 0
   self._root_proxy = ProxyContainer(nil, 'stonehearth:entities:building')

   local entity = self._root_proxy:get_entity()
   entity:add_component('stonehearth:no_construction_zone')
end

function ProxyBuilder:set_brush(type, uri)
   self._brushes[type] = uri
end

function ProxyBuilder:_clear()
   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end
   self._root_proxy:destroy()
   self._root_proxy = ProxyContainer(nil)
   self._columns = nil
   self._walls = nil
end

function ProxyBuilder:_start()
   assert(not self._columns)
   assert(not self._walls)
   self._columns = {}
   self._walls = {}
   
   self._input_capture = stonehearth.input:capture_input()
                           :on_mouse_event(function(e)
                                 return self._on_mouse(self._derived, e)
                              end)
                           :on_keyboard_event(function(e)
                                 return self._on_keyboard(self._derived, e)
                              end)

end

function ProxyBuilder:move_to(location)
   self._root_proxy:move_to(location)
end

function ProxyBuilder:turn_to(angle)
   self._root_proxy:turn_to(angle)
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
   local column = ProxyColumn(self._root_proxy, self._brushes.column)
   table.insert(self._columns, column)
   return column
end

function ProxyBuilder:add_wall()
   local wall = ProxyWall(self._root_proxy, self._brushes.wall)
   table.insert(self._walls, wall)
   return wall
end

function ProxyBuilder:add_door()
   return ProxyPortal(self._root_proxy, self._brushes.door)
end

function ProxyBuilder:add_roof()
   return ProxyRoof(self._root_proxy, self._brushes.roof)
end

function ProxyBuilder:shift_down()
  return _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_LEFT_SHIFT) or
         _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_RIGHT_SHIFT)
end

--- Add all descendants of proxy to the 'order' list in package order.
-- The client is responsible for providing a list of proxies to the
-- server in an order that's trivial to consume.  This means proxies
-- must be defined before they can be referenced.  No problem!  Just do
-- a post-order traversal of the proxy tree.  Since we're concerned about
-- both the 'children' and 'dependency' trees, this is actually a weird
-- directed graph (which means we need a visited map to keep track of where
-- we've been).
function ProxyBuilder:_compute_package_order(visited, order, proxy)
   local id = proxy:get_id()

   if not visited[id] then
      visited[id] = true

      for id, child in pairs(proxy:get_children()) do
         self:_compute_package_order(visited, order, child)
      end
      for id, dependency in pairs(proxy:get_dependencies()) do
         self:_compute_package_order(visited, order, dependency)
      end
      table.insert(order, proxy)
   end
end

--- Package a single proxy for deliver to the server
function ProxyBuilder:_package_proxy(proxy)
   local package = {}

   local entity =  proxy:get_entity()  
   package.entity_uri = entity:get_uri()
   package.entity_id = entity:get_id()
   
   local mob = entity:get_component('mob')   
   if mob then
      local parent = mob:get_parent()
      if not parent then
         package.add_to_build_plan = true
      end
   end
   
   -- Package up the trival components...
   package.components = {}
   for _, name in ipairs({'mob', 'destination'}) do
      local component = entity:get_component(name)
      package.components[name] = component and component:serialize() or {}
   end   
   
   -- Package up the stonehearth:construction_data component.  This is almost
   -- trivial, but not quite.
   local datastore = entity:get_component('stonehearth:construction_data')
   if datastore then
      -- remove the paint_mode.  we just set it here so the blueprint
      -- would be rendered differently
      local data = datastore:get_data()
      data.paint_mode = nil
      package.components['stonehearth:construction_data'] = data
   end

   local building = proxy:get_building()
   if building then
      package.building_id = building:get_id()
   end
   
   -- Package the child and dependency lists.  Build it as a table first to
   -- make sure we don't get duplicates in the children/dependency list,
   -- then shove it into the package.dependencies
   local all_dependencies = {}
   local children = proxy:get_children()
   if next(children) then
      package.children = {}
      for id, child in pairs(children) do
         -- TODO(tony): fix this!  Hack to work around adding portals as dependencies of proxies.
         if child:get_entity():get_component('stonehearth:portal') == nil then
            all_dependencies[id] = true
         end
         table.insert(package.children, id)
      end
   end
   
   local dependencies = proxy:get_dependencies()
   if next(dependencies) then
      for id, _ in pairs(dependencies) do
         all_dependencies[id] = true
      end
   end

   if next(all_dependencies) then
      package.dependencies = {}
      for id, _ in pairs(all_dependencies) do
         table.insert(package.dependencies, id)
      end
   end

   -- borrowed scaffolding...
   local borrowing_scaffolding = proxy:get_loaning_scaffolding_to()
   if next(borrowing_scaffolding) then
      package.loan_scaffolding_to = {}
      for id, _ in pairs(borrowing_scaffolding) do
         table.insert(package.loan_scaffolding_to, id)
      end
   end


   return package
end

function ProxyBuilder:publish()
   local changed = {}
   self:_compute_package_order({}, changed, self._root_proxy)
   for i, proxy in ipairs(changed) do
      changed[i] = self:_package_proxy(proxy)
   end

   _radiant.call('stonehearth:build_structures', changed)
   --TODO: you COULD put this in an "always" so the capture doesn't
   --go away till the fabricator appears, but the gap is so long
   --it looks like a bug
   self:_clear()
end

function ProxyBuilder:cancel()
   self:_clear()
end

function ProxyBuilder:rotate()
   self._rotation = self._rotation + 90
   if self._rotation >= 360 then
      self._rotation = 0
   end
   self:turn_to(self._rotation)
   return self
end

return ProxyBuilder
