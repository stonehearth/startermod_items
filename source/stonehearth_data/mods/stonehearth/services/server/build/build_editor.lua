local BuildEditor = class()
local StructureEditor = class()
-- xxx: move all the proxy stuff to the client! - tony
local ProxyWall = require 'services.server.build.proxy_wall'
local ProxyWallBuilder = require 'services.server.build.proxy_wall_builder'
local ProxyFloorBuilder = require 'services.server.build.proxy_floor_builder'
local ProxyRoomBuilder = require 'services.server.build.proxy_room_builder'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')

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

local function get_fbp_for_structure(entity, structure_component_name)
   local fabricator, blueprint, project = get_fbp_for(entity)
   if blueprint then
      local structure_component = blueprint:get_component(structure_component_name)
      if structure_component then
         return fabricator, blueprint, project, structure_component
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

function StructureEditor:__init(fabricator, blueprint, project, structure_uri)
   self._fabricator = fabricator
   self._blueprint = blueprint
   self._project = project

   local building = get_building_for(blueprint)
   local location = radiant.entities.get_world_grid_location(building)
   self._proxy_building = radiant.entities.create_entity(building:get_uri())
   self._proxy_building:set_debug_text('proxy building')
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
   self._proxy_blueprint:set_debug_text('proxy blueprint')
   self._proxy_blueprint:add_component('destination')
                           :set_region(rgn)

   local structure = blueprint:get_component(structure_uri)
   assert(structure)
   local proxy_structure = self._proxy_blueprint:add_component(structure_uri)
                                                   :begin_editing(structure)
                                                   :layout()
   local editing_reserved_region = proxy_structure:get_editing_reserved_region()
   
   self._proxy_blueprint:add_component('stonehearth:construction_data')
                           :begin_editing(blueprint:get_component('stonehearth:construction_data'))

   self._proxy_fabricator = radiant.entities.create_entity(fabricator:get_uri())
   self._proxy_fabricator :set_debug_text('proxy fabricator')

   rgn = _radiant.client.alloc_region()
   rgn:modify(function(cursor)
         local source_rgn = fabricator:get_component('destination'):get_region()
         cursor:copy_region(source_rgn:get())
      end)
   self._proxy_fabricator:add_component('destination')
                           :set_region(rgn)
   
   self._proxy_fabricator:add_component('stonehearth:fabricator')
      :begin_editing(self._proxy_blueprint, project, editing_reserved_region)

   self._proxy_blueprint:add_component('stonehearth:construction_progress')
                           :begin_editing(self._proxy_building, self._proxy_fabricator)

   radiant.entities.add_child(self._proxy_building, self._proxy_blueprint, blueprint:get_component('mob'):get_grid_location())
   radiant.entities.add_child(self._proxy_building, self._proxy_fabricator, fabricator:get_component('mob'):get_grid_location() + Point3(0, 0, 0))

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

function StructureEditor:get_blueprint()
   return self._blueprint
end

function StructureEditor:get_proxy_blueprint()
   return self._proxy_blueprint
end

function StructureEditor:get_proxy_fabricator()
   return self._proxy_fabricator
end

function StructureEditor:get_world_origin()
   return radiant.entities.get_world_grid_location(self._proxy_fabricator)
end

function StructureEditor:should_keep_focus(entity)
   if entity == self._proxy_blueprint or 
      entity == self._proxy_fabricator or 
      entity == self._project then
      return true
   end
-- keep going up until we find one!
   local mob = entity:get_component('mob')
   if mob then
      local parent = mob:get_parent()
      if parent then
         log:detail('checking parent... %s', parent)
         return self:should_keep_focus(parent)
      end
   end
   return false
end

local WallEditor = class(StructureEditor)

function WallEditor:__init(fabricator, blueprint, project)
   self[StructureEditor]:__init(fabricator, blueprint, project, 'stonehearth:wall')
   self._wall = self:get_proxy_blueprint():get_component('stonehearth:wall')
end

function WallEditor:destroy()  
   if self._portal then
      log:detail('destroying portal %s', tostring(self._portal))
      radiant.entities.destroy_entity(self._portal)
      self._portal = nil
   end
   self[StructureEditor]:destroy()
end

function WallEditor:set_portal_uri(uri)
   self._portal_uri = uri
   self._portal = radiant.entities.create_entity(uri)
   self._portal:add_component('render_info')
                  :set_material('materials/ghost_item.xml')

   self._wall:add_portal(self._portal)
   return self
end

function WallEditor:on_mouse_event(e, selection)
   local location
   local proxy_fabricator = self:get_proxy_fabricator()
   for result in selection:each_result() do
      if result.entity == proxy_fabricator then
         location = result.brick
      end
   end
   if location then
      location = location - self:get_world_origin()      
      location.y = 0
      location[self._wall:get_normal_coord()] = 0
      radiant.entities.move_to(self._portal, location)
      self._wall:layout()
   end      
end

function WallEditor:submit(response)   
   local location = self._portal:get_component('mob'):get_grid_location()
   _radiant.call('stonehearth:add_portal', self:get_blueprint(), self._portal_uri, location)
      :done(function(r)
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
      :always(function()
            self:destroy()
         end)
end

function BuildEditor:add_door(session, response)
   local wall_editor
   local capture = stonehearth.input:capture_input()

   capture:on_mouse_event(function(e)
         local s = _radiant.client.query_scene(e.x, e.y)
         if s and s:get_result_count() > 0 then            
            local entity = s:get_result(0).entity
            if entity and entity:is_valid() then
               log:detail('hit entity %s', entity)
               if wall_editor and not wall_editor:should_keep_focus(entity) then
                  log:detail('destroying wall editor')
                  wall_editor:destroy()
                  wall_editor = nil
               end
               if not wall_editor then
                  local fabricator, blueprint, project = get_fbp_for_structure(entity, 'stonehearth:wall')
                  log:detail('got blueprint %s', tostring(blueprint))
                  if blueprint then
                     log:detail('creating wall editor for blueprint: %s', blueprint)
                     wall_editor = WallEditor(fabricator, blueprint, project)
                                       :set_portal_uri('stonehearth:wooden_door')
                  end
               end
               if wall_editor then
                  wall_editor:on_mouse_event(e, s)
               end
            end
         end
         if e:up(1) then
            if wall_editor then
               wall_editor:submit(response)
            end
            capture:destroy()
         end
         return true
      end) 
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
            if s and s:get_result_count() > 0 then
               local entity = s:get_result(0).entity
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

function BuildEditor:create_room(session, response)
   ProxyRoomBuilder():go()  
   return true
end

return BuildEditor
