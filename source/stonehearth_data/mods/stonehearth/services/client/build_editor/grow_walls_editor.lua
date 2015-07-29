local build_util = require 'lib.build_util'
local constants = require('constants').construction
local StructureEditor = require 'services.client.build_editor.structure_editor'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3


local STOREY_HEIGHT = stonehearth.constants.construction.STOREY_HEIGHT

local log = radiant.log.create_logger('build_editor.grow_walls')
local GrowWallsEditor = class()

-- this is the component which manages the fabricator entity.
function GrowWallsEditor:__init(build_service)
   log:debug('created')

   self._build_service = build_service

   self._preview_walls = {}
   self._preview_columns = {}
end

function GrowWallsEditor:destroy()
   log:debug('destroyed')
   self:_destroy_preview_entities()
end

function GrowWallsEditor:_destroy_preview_entities()
   for _, editor in pairs(self._preview_walls) do
      editor:destroy()
   end
   for _, editor in pairs(self._preview_columns) do
      editor:destroy()
   end
   self._preview_walls = {}
   self._preview_columns = {}
end

function GrowWallsEditor:go(response, column_brush, wall_brush)
   log:detail('running')
   stonehearth.selection:select_entity_tool()
      :set_cursor('stonehearth:cursors:grow_walls')
      :set_filter_fn(function(result)
            local entity = result.entity
            if radiant.entities.is_temporary_entity(entity) then
               -- one of our client-side preview entities.  bail.
               return stonehearth.selection.FILTER_IGNORE
            end
            local blueprint = build_util.get_blueprint_for(entity)
            if not blueprint then
               -- not a blueprint, bail.
               return false
            end
            local building = build_util.get_building_for(blueprint)
            if not building then
               -- no building for this blueprint.  weird!  bail.
               return false
            end
            local floor = blueprint:get_component('stonehearth:floor')
            if not floor then
               -- only grow around floor
               return false
            end

            if floor:get().category ~= constants.floor_category.FLOOR then
               -- never grow around roads
               return false
            end

            -- try to grow...
            self:_try_grow_walls(entity, result.brick)

            return self._last_walls ~= nil
         end)
      :progress(function(selector, entity, result)
            if not entity then
               self._last_point = nil
               self._last_target = nil
               self._last_walls = nil
               self._last_walls_desc = nil
               self._cursor_dirty = false
               self:_destroy_preview_entities()
               return
            end
            
            self:_recreate_cursor(column_brush, wall_brush)
         end)
      :done(function(selector, entity, result)
            log:detail('box selected')
            if entity then
               _radiant.call_obj(self._build_service, 'grow_walls_command', entity, column_brush, wall_brush, result.brick)
                           :done(function(r)
                                 if r.new_selection then
                                    stonehearth.selection:select_entity(r.new_selection)
                                 end
                              end)
                           :always(function()
                                 self:destroy()
                              end)
            else
               self:destroy()
            end
            response:resolve('done')
         end)
      :fail(function(selector)
            log:detail('failed to select building')
            response:reject('failed')
            self:destroy()
         end)      
      :go()

   return self
end

-- NOTE: client-side components only!  That's why this is here, and not in build_util.
-- Iffy.  Right now, my thinking is roads can only be merged with other roads, and roads
-- cannot have walls grown on them (and therefore no roofs).  Maybe they can have fixtures?
-- (Lampposts, other doodads.)
function GrowWallsEditor:is_road(building)
   for _, floor in pairs(building:get_component('stonehearth:building'):get_floors()) do
      local floor_type = floor:get_component('stonehearth:floor'):get().category
      if floor_type == constants.floor_category.ROAD or floor_type == constants.floor_category.CURB then
         return true
      end
   end
   return false
end

function GrowWallsEditor:_try_grow_walls(target, pt)
   checks('self', 'Entity', 'Point3')
   if target ~= self._last_target or pt ~= self._last_point then
      local origin = radiant.entities.get_world_grid_location(target)
      local offset = pt - origin

      local walls = {}
      local walls_desc = ''
      build_util.grow_walls_around(target, offset, function(min, max, normal)
            table.insert(walls, { min, max, normal })
            walls_desc = walls_desc .. string.format('min=%s,max=%s,normal=%s ',
                                                      tostring(min), tostring(max), tostring(normal))
         end)

      if walls_desc ~= self._last_walls_desc then
         self._cursor_dirty = true
         self._last_walls_desc = walls_desc

         for i, wall in ipairs(walls) do
            local min, max, normal = unpack(wall)
            if self:_wall_obstructed(min, max) then
               self._last_walls = nil
               return
            end
         end
         self._last_walls = walls
      end
   end
end

function GrowWallsEditor:_recreate_cursor(column_brush, wall_brush)
   if not self._cursor_dirty then 
      return
   end

   self._cursor_dirty = false
   self:_destroy_preview_entities()
   if self._last_walls then
      for i, wall in ipairs(self._last_walls) do
         local min, max, normal = unpack(wall)            
         local col_a = self:_create_preview_column(min, column_brush)
         local col_b = self:_create_preview_column(max, column_brush)
         if col_a and col_b then
            if min ~= max then
               self:_create_preview_wall(col_a, col_b, normal, wall_brush)
            end
         end
      end
   end
end

function GrowWallsEditor:_wall_obstructed(min, max)
   local query_cube = Cube3(min, max + Point3(1, STOREY_HEIGHT, 1))
   for _, entity in pairs(radiant.terrain.get_entities_in_cube(query_cube)) do
      if entity:get_id() == 1 then
         return true
      end
      local blueprint = build_util.get_blueprint_for(entity)
      if blueprint then
         return true
      end
      local rcs = entity:get_component('region_collision_shape')
      if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
         return true
      end
   end
   return false
end

function GrowWallsEditor:_create_preview_column(pt, column_brush)
   local key = pt:key_value()
   local editor = self._preview_columns[key]
   if not editor  then
      editor = StructureEditor()
      self._preview_columns[key] = editor

      local column = editor:create_blueprint('stonehearth:build:prototypes:column', 'stonehearth:column')
      column:get_component('stonehearth:column')
               :set_brush(column_brush)
               :layout()

      editor:move_to(pt)
   end
   return editor:get_proxy_blueprint()
end

function GrowWallsEditor:_create_preview_wall(column_a, column_b, normal, wall_brush)
   local editor = StructureEditor()
   editor:create_blueprint('stonehearth:build:prototypes:wall', 'stonehearth:wall')
   table.insert(self._preview_walls, editor)
   
   local wall = editor:get_proxy_blueprint()
   wall:add_component('stonehearth:wall')
            :set_brush(wall_brush)
            :connect_to_columns(column_a, column_b, normal)
            :layout()

   wall:add_component('stonehearth:construction_data')
            :set_normal(normal)

   -- sigh.  i hate that we have to do this...
   editor:move_to(wall:get_component('mob'):get_grid_location())
   return wall
end

return GrowWallsEditor

