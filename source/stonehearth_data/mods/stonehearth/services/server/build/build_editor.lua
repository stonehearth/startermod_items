local BuildEditor = class()
local StructureEditor = class()
-- xxx: move all the proxy stuff to the client! - tony
local ProxyWall = require 'services.server.build.proxy_wall'
local ProxyWallBuilder = require 'services.server.build.proxy_wall_builder'
local ProxyFloorBuilder = require 'services.server.build.proxy_floor_builder'
local ProxyRoomBuilder = require 'services.server.build.proxy_room_builder'
local Point3 = _radiant.csg.Point3


local function get_fbp_for(entity)
   if entity and entity:is_valid() then
      -- is this a fabricator?  if so, finding the blueprint and the project is easy!
      local fc = entity:get_component('stonehearth:fabricator')
      if fc then
         return entity, fc:get_blueprint(), fc:get_project()
      end

      -- is this a blueprint or project?  use the construction_data component
      -- to get back the fabricator
      local cd = entity:get_component('stonehearth:construction_data')
      if cd then
         return get_fbp_for(cd:get_fabricator_entity())
      end
   end
end

local function get_building_for(entity)
   if entity and entity:is_valid() then
      local _, blueprint, _ = get_fbp_for(entity)      
      local cp = blueprint:get_component('stonehearth:construction_progress')
      if cp then
         return cp:get_building_entity()
      end
   end
end


local STRUCTURE_COMPONENTS = {
   'stonehearth:wall'
}
function StructureEditor:__init(fabricator, blueprint, project)
   self._fabricator = fabricator
   self._blueprint = blueprint
   self._project = project

   local building = get_building_for(blueprint)
   local location = radiant.entities.get_world_grid_location(building)
   self._proxy_building = radiant.entities.create_entity(building:get_uri())
   self._proxy_building:add_component('mob')
                           :set_location_grid_aligned(location)
   blueprint:add_component('unit_info')
            :set_player_id(radiant.entities.get_player_id(building))
            :set_faction(radiant.entities.get_faction(building))

   local rgn = _radiant.client.alloc_region()
   rgn:modify(function(cursor)
         local source_rgn = blueprint:get_component('destination'):get_region()
         cursor:copy_region(source_rgn:get())
      end)
   self._proxy_blueprint = radiant.entities.create_entity(blueprint:get_uri())
   self._proxy_blueprint:add_component('destination')
                           :set_region(rgn)

   for _, name in ipairs(STRUCTURE_COMPONENTS) do
      local structure = blueprint:get_component(name)
      if structure then
         self._proxy_blueprint:add_component(name)
                                    :begin_editing(structure)
      end
   end   
   self._proxy_blueprint:add_component('stonehearth:construction_data')
                           :begin_editing(blueprint:get_component('stonehearth:construction_data'))
   self._proxy_blueprint:add_component('stonehearth:construction_progress')
                           :begin_editing(self._proxy_building, self._proxy_fabricator)

   self._proxy_fabricator = radiant.entities.create_entity(fabricator:get_uri())
   self._proxy_fabricator:add_component('stonehearth:fabricator')
      :begin_editing(self._proxy_blueprint, project)
   rgn = _radiant.client.alloc_region()
   rgn:modify(function(cursor)
         local source_rgn = fabricator:get_component('destination'):get_region()
         cursor:copy_region(source_rgn:get())
      end)
   self._proxy_fabricator:add_component('destination')
                           :set_region(rgn)
   

   radiant.entities.add_child(self._proxy_building, self._proxy_blueprint, blueprint:get_component('mob'):get_grid_location())
   radiant.entities.add_child(self._proxy_building, self._proxy_fabricator, fabricator:get_component('mob'):get_grid_location())

   self._render_entity = _radiant.client.create_render_entity(1, self._proxy_building)

   -- hide the fabricator and structure...
   _radiant.client.get_render_entity(self._fabricator):set_visible(false)
   _radiant.client.get_render_entity(self._project):set_visible(false)
end

function StructureEditor:destroy()
   -- hide the fabricator and structure...
   radiant.entities.destroy_entity(self._proxy_fabricator)
   radiant.entities.destroy_entity(self._proxy_blueprint)
   radiant.entities.destroy_entity(self._proxy_building)

   _radiant.client.get_render_entity(self._fabricator):set_visible(true)
   _radiant.client.get_render_entity(self._project):set_visible(true)
end

function BuildEditor:__init()
   self._model = _radiant.client.create_datastore()
   self._model:set_controller(self)
end

function BuildEditor:get_model()
   return self._model
end

function BuildEditor:place_new_wall(session, response)
   ProxyWallBuilder():go(response)
end

function BuildEditor:place_new_floor(session, response)
   ProxyFloorBuilder():go(response)
end

function BuildEditor:grow_walls(session, response)
   local capture = stonehearth.input:capture_input()
   capture:on_mouse_event(function(e)
         if e:up(1) then
            local s = _radiant.client.query_scene(e.x, e.y)
            if s then
               local entity = radiant.get_object(s:objectid_of(0))
               if entity and entity:is_valid() then
                  local building = get_building_for(entity)
                  if building then
                     _radiant.call('stonehearth:grow_walls', building, 'stonehearth:wooden_column', 'stonehearth:wooden_wall')
                        :done(function(r)
                              response:resolve(r)
                           end)
                        :fail(function(r)
                              response:reject(r)
                           end)
                     capture:destroy()
                     return
                  end
               end
            end
            response:reject({ error = 'unknown error' })
            capture:destroy()
         end
         return true
      end)   
end

function BuildEditor:grow_roof(session, response)
   local capture = stonehearth.input:capture_input()
   capture:on_mouse_event(function(e)
         if e:up(1) then
            local s = _radiant.client.query_scene(e.x, e.y)
            if s then
               local entity = radiant.get_object(s:objectid_of(0))
               if entity and entity:is_valid() then
                  local building = get_building_for(entity)
                  if building then
                     _radiant.call('stonehearth:grow_roof', building, 'stonehearth:wooden_peaked_roof')
                        :done(function(r)
                              response:resolve(r)
                           end)
                        :fail(function(r)
                              response:reject(r)
                           end)
                     capture:destroy()
                     return
                  end
               end
            end
            response:reject({ error = 'unknown error' })
            capture:destroy()
         end
         return true
      end)   
end

function BuildEditor:add_door(session, response)
   local wall_editor
   local capture = stonehearth.input:capture_input()
   capture:on_mouse_event(function(e)
         local s = _radiant.client.query_scene(e.x, e.y)
         if s and s:num_results() > 0 then
            local entity = radiant.get_object(s:objectid_of(0))
            if entity and entity:is_valid() then
               local fabricator, blueprint, project = get_fbp_for(entity)
               if blueprint then
                  local wall_data = blueprint:get_component('stonehearth:wall')
                  if wall_data then
                     if wall_editor then
                        wall_editor:destroy()
                     end
                     wall_editor = StructureEditor(fabricator, blueprint, project)
                  end
               end
            end
         end
         if e:up(1) then
            response:reject({ error = 'unknown error' })
            capture:destroy()
         end
         return true
      end)   
end

function BuildEditor:create_room(session, response)
   ProxyRoomBuilder():go()  
   return true
end

return BuildEditor
