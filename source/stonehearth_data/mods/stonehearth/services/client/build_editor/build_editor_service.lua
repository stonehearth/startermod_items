local build_util = require 'lib.build_util'
-- xxx: move all the proxy stuff to the client! - tony
local StructureEditor = require 'services.client.build_editor.structure_editor'
local FloorEditor = require 'services.client.build_editor.floor_editor'
local GrowWallsEditor = require 'services.client.build_editor.grow_walls_editor'
local GrowRoofEditor = require 'services.client.build_editor.grow_roof_editor'
local RoadEditor = require 'services.client.build_editor.road_editor'
local StructureEraser = require 'services.client.build_editor.structure_eraser'
local PortalEditor = require 'services.client.build_editor.portal_editor'
local WallLoopEditor = require 'services.client.build_editor.wall_loop_editor'
local DoodadPlacer = require 'services.client.build_editor.doodad_placer'
local LadderEditor = require 'services.client.build_editor.ladder_editor'
local TemplateEditor = require 'services.client.build_editor.template_editor'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')

local BuildEditorService = class()

function BuildEditorService:initialize()
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
   self._sel_changed_listener = radiant.events.listen(radiant, 'stonehearth:selection_changed', self, self.on_selection_changed)
end

function BuildEditorService:on_selection_changed()
   local selected = nil
   local maybe_selected = stonehearth.selection:get_selected()
   local old_selected = self._sv.selected_sub_part
   local building_entity

   if maybe_selected then
      local fab = maybe_selected:get_component('stonehearth:fabricator')
      if fab then
         local bp = fab:get_blueprint()
         if bp then
            local cpc = bp:get_component('stonehearth:construction_progress')
            if cpc then
               building_entity = cpc:get_building_entity()
               if building_entity then
                  selected = maybe_selected
               end
            end
         end
      else
         local bc = maybe_selected:get_component('stonehearth:building')
         if bc then
            building_entity = maybe_selected
         end
      end
   end

   if building_entity then
      self._sel_changed_listener:destroy()
      stonehearth.selection:select_entity(building_entity)
      self._sel_changed_listener = radiant.events.listen(radiant, 'stonehearth:selection_changed', self, self.on_selection_changed)
   end

   if old_selected == selected then
      return
   end

   if old_selected and not old_selected:is_valid() then
      old_selected = nil
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

function BuildEditorService:place_new_wall(session, response, column_brush, wall_brush)
   WallLoopEditor(self._build_service)
         :go(column_brush, wall_brush, response)
end

function BuildEditorService:place_new_floor(session, response, brush, sink_floor)
   FloorEditor(self._build_service)
         :go(response, brush, { sink_floor = sink_floor })
end

function BuildEditorService:place_new_slab(session, response, slab_shape)
   FloorEditor(self._build_service)
         :go(response, slab_shape, { sink_floor = false })
end

function BuildEditorService:erase_structure(session, response, brush_shape)
   StructureEraser(self._build_service)
         :go(response)
end

function BuildEditorService:place_new_road(session, response, road_brush_shape)
   RoadEditor(self._build_service)
         :go(response, road_brush_shape)
end

function BuildEditorService:place_template(session, response, template_name)
   TemplateEditor(self._build_service)
         :go(response, template_name)
end

function BuildEditorService:grow_walls(session, response, column_brush, wall_brush)
   GrowWallsEditor(self._build_service)
         :go(response, column_brush, wall_brush)
end

function BuildEditorService:set_grow_roof_options(session, response, options)
   if self._grow_roof_editor then
      self._grow_roof_editor:apply_options(options)
   end
   return true
end

function BuildEditorService:grow_roof(session, response, roof_brush, options)
   self._grow_roof_editor = GrowRoofEditor(self._build_service)
   self._grow_roof_editor:go(response, roof_brush, options)
   
   --[[
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
      :set_filter_fn(function(result)
            local entity = result.entity
            building = build_util.get_building_for(entity)
            return building ~= nil and not has_roof_fn(building) and build_util.has_walls(building)
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
            response:resolve('done')
         end)
      :fail(function(selector)
            response:reject('failed')
         end)
      :go()   
      ]]
end

return BuildEditorService
