local BuildEditorService = class()
-- xxx: move all the proxy stuff to the client! - tony
local StructureEditor = require 'services.client.build_editor.structure_editor'
local FloorEditor = require 'services.client.build_editor.floor_editor'
local FloorEraser = require 'services.client.build_editor.floor_eraser'
local PortalEditor = require 'services.client.build_editor.portal_editor'
local WallLoopEditor = require 'services.client.build_editor.wall_loop_editor'
local DoodadPlacer = require 'services.client.build_editor.doodad_placer'
local LadderEditor = require 'services.client.build_editor.ladder_editor'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')


function BuildEditorService:initialize()
   self._grow_roof_options = {}
   self.__saved_variables:set_controller(self)
   self._sv = self.__saved_variables:get_data()
   self._sv.selected_sub_part = nil

   _radiant.call('stonehearth:get_service', 'build')
      :done(function(r)
            -- we'd like to have just received the address instead of needing
            -- to manually convert it here, but even if the address was sent, 
            -- the client will try to convert it back to a DataStore before handing
            -- it to us =(
            self._build_service = r.result:__tojson()
         end)
   radiant.events.listen(radiant, 'stonehearth:selection_changed', self, self.on_selection_changed)
end

function BuildEditorService:on_selection_changed()
   local selected = nil
   local old_selected = self._sv.selected_sub_part
   local building_entity

   if stonehearth.selection:get_selected() then
     local fab = stonehearth.selection:get_selected():get_component('stonehearth:fabricator')
     if fab then
       local bp = fab:get_blueprint()
       if bp then
         local cpc = bp:get_component('stonehearth:construction_progress')
         if cpc then
           building_entity = cpc:get_building_entity()
           if building_entity then
              selected = stonehearth.selection:get_selected()
           end
         end
       end
     end
   end

   if building_entity then
     radiant.events.unlisten(radiant, 'stonehearth:selection_changed', self, self.on_selection_changed)
     stonehearth.selection:select_entity(building_entity)
     radiant.events.listen(radiant, 'stonehearth:selection_changed', self, self.on_selection_changed)
   end

   if old_selected == selected then
      return
   end

   self._sv.selected_sub_part = selected
   self.__saved_variables:mark_changed()

   radiant.events.trigger(self, 'stonehearth:sub_selection_changed', 
      {
         old_selection = old_selected,
         new_selection = selected
      })
end

function BuildEditorService:get_sub_selection()
   return self._sv.selected_sub_part
end

function BuildEditorService:build_ladder(session, response)
   LadderEditor(self._build_service)
         :go(session, response)
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
   local has_wall_fn = function(building)
      for _, child in building:get_component('entity_container'):each_child() do
        if child:get_component('stonehearth:wall') then
          return true
         end
      end
      return false
   end
   local building
   stonehearth.selection:select_entity_tool()
      :set_cursor('stonehearth:cursors:grow_walls')
      :set_filter_fn(function(entity)
            building = self:get_building_for(entity)
            return building ~= nil and not has_wall_fn(building)
         end)
      :done(function(selector, entity)
            if building then
               _radiant.call_obj(self._build_service, 'grow_walls_command', building, columns_uri, walls_uri)
            end
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('failed')
         end)
      :always(function(selector)
            selector:destroy()
            response:resolve('done')
         end)
      :go()
end

function BuildEditorService:set_grow_roof_options(session, response, options)
   self._grow_roof_options = options
   return true
end

function BuildEditorService:grow_roof(session, response, roof_uri)
   local has_roof_fn = function(building)
      for _, child in building:get_component('entity_container'):each_child() do
         if child:get_component('stonehearth:roof') then   
            return true
         end
      end
      return false
   end

   local building
   stonehearth.selection:select_entity_tool()
      :set_cursor('stonehearth:cursors:grow_roof')
      :set_filter_fn(function(entity)
            building = self:get_building_for(entity)
            return building ~= nil and not has_roof_fn(building)
         end)
      :done(function(selector, entity)
            if building then
               _radiant.call_obj(self._build_service, 'grow_roof_command', building, roof_uri, self._grow_roof_options)
                  :done(function(r)
                        if r.new_selection then
                           stonehearth.selection:select_entity(r.new_selection)
                        end
                     end)
            end
         end)
      :fail(function(selector)
         selector:destroy()
         response:reject('failed')
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
