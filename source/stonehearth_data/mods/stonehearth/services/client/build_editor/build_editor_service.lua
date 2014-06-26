local BuildEditorService = class()
-- xxx: move all the proxy stuff to the client! - tony
local StructureEditor = require 'services.client.build_editor.structure_editor'
local FloorEditor = require 'services.client.build_editor.floor_editor'
local FloorEraser = require 'services.client.build_editor.floor_eraser'
local PortalEditor = require 'services.client.build_editor.portal_editor'
local WallLoopEditor = require 'services.client.build_editor.wall_loop_editor'
local DoodadPlacer = require 'services.client.build_editor.doodad_placer'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')


function BuildEditorService:initialize()
   self.__saved_variables:set_controller(self)

   _radiant.call('stonehearth:get_service', 'build')
      :done(function(r)
            -- we'd like to have just received the address instead of needing
            -- to manually convert it here, but even if the address was sent, 
            -- the client will try to convert it back to a DataStore before handing
            -- it to us =(
            self._build_service = r.result:__tojson()
         end)
end

function BuildEditorService:add_doodad(session, response, uri)
   DoodadPlacer(self._build_service)
         :go(session, response, uri)
end

function BuildEditorService:place_new_wall(session, response, columns_uri, walls_uri)
   WallLoopEditor(self._build_service)
         :go(columns_uri, walls_uri, response)
end

function BuildEditorService:place_new_floor(session, response, brush_shape)
   FloorEditor(self._build_service)
         :go(response, brush_shape)
end

function BuildEditorService:erase_floor(session, response, brush_shape)
   FloorEraser(self._build_service)
         :go(response)
end

function BuildEditorService:grow_walls(session, response, columns_uri, walls_uri)
   local building
   stonehearth.selection:select_entity_tool()
      :set_tool_mode(true)
      :set_cursor('stonehearth:cursors:grow_walls')
      :set_filter_fn(function(entity)
            building = self:get_building_for(entity)
            return building ~= nil
         end)
      :done(function(selector, entity)
            if building then
               _radiant.call_obj(self._build_service, 'grow_walls_command', building, columns_uri, walls_uri)
            end
         end)
      :always(function(selector)
            selector:destroy()
            response:resolve('done')
         end)
      :go()
end

function BuildEditorService:grow_roof(session, response)
   local building
   stonehearth.selection:select_entity_tool()
      :set_tool_mode(true)
      :set_cursor('stonehearth:cursors:grow_roof')
      :set_filter_fn(function(entity)
            building = self:get_building_for(entity)
            return building ~= nil
         end)
      :done(function(selector, entity)
            if building then
               _radiant.call_obj(self._build_service, 'grow_roof_command', building, 'stonehearth:wooden_peaked_roof')
            end
         end)
      :always(function(selector)
            selector:destroy()
            response:resolve('done')
         end)
      :go()   
end

function BuildEditorService:create_room(session, response)
   response:fail({ error = 'remove me!'})
end


function BuildEditorService:get_fbp_for(entity)
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
         return self:get_fbp_for(cd:get_fabricator_entity())
      end
   end
end

function BuildEditorService:get_fbp_for_structure(entity, structure_component_name)
   local fabricator, blueprint, project = self:get_fbp_for(entity)
   if blueprint then
      local structure_component = blueprint:get_component(structure_component_name)
      if structure_component then
         return fabricator, blueprint, project, structure_component
      end
   end
end


function BuildEditorService:get_building_for(entity)
   if entity and entity:is_valid() then
      local _, blueprint, _ = self:get_fbp_for(entity)
      if blueprint then
         local cp = blueprint:get_component('stonehearth:construction_progress')
         if cp then
            return cp:get_building_entity()
         end
      end
   end
end

return BuildEditorService
