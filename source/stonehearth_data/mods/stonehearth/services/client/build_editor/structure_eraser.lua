local build_util = require 'lib.build_util'

local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local StructureEraser = class()

-- this is the component which manages the fabricator entity.
function StructureEraser:__init(build_service)
   self._build_service = build_service
end

function StructureEraser:go(response)   
   stonehearth.selection:register_tool(self, true)
   self._cursor_obj = _radiant.client.set_cursor('stonehearth:cursors:invalid_hover')

   self._response = response   
   self:_add_tool_selection_capture()
end

function StructureEraser:_cleanup()
   stonehearth.selection:register_tool(self, false)

   self:_remove_tool_selection_capture()

   if self._cursor_obj then
      self._cursor_obj:destroy()
      self._cursor_obj = nil
   end
   if self._current_tool then
      self._current_tool:destroy()
      self._current_tool = nil
   end   
end

function StructureEraser:resolve(...)
   if self._response then
      self._response:resolve(...)
      self._response = nil
   end
   self:_cleanup()
end

function StructureEraser:reject(...)
   if self._response then
      self._response:reject(...)
      self._response = nil
   end
   self:_cleanup()
end

function StructureEraser:_choose_tool(mouse)
   local tool

   local s = _radiant.client.query_scene(mouse.x, mouse.y)
   for result in s:each_result() do
      local entity = result.entity
      if entity and entity:is_valid() then
         -- if they're mousing over the terrain, use the erase_floor tool.
         if entity:get_id() == 1 then
            return 'erase_floor', entity            
         end

         local blueprint = build_util.get_blueprint_for(entity)
         if blueprint then
            -- overing over a fixture?  use erase_fixture!
            if blueprint:get_component('stonehearth:fixture_fabricator') then
               return 'erase_fixture', entity
            end
            -- overing over the floor?  use erase_floor!
            if blueprint:get_component('stonehearth:floor') then
               return 'erase_floor', entity
            end

            -- no other blueprint types can be erased.  remove the current tool.
            return nil
         end
      end
   end
end

function StructureEraser:_install_correct_tool(mouse)
   local tool_name, hover_entity = self:_choose_tool(mouse)
   if tool_name ~= self._current_tool_name then
      self:_switch_tool(tool_name, hover_entity)
      return true
   end
   return false
end

function StructureEraser:_remove_tool_selection_capture()
   if self._tool_selection_capture then
      self._tool_selection_capture:destroy()
      self._tool_selection_capture = nil
   end
end

function StructureEraser:_add_tool_selection_capture()
   self._tool_selection_capture = stonehearth.input:capture_input()
                                       :on_mouse_event(function(event)
                                             if stonehearth.selection.user_cancelled(event) then
                                                self:reject({ error = 'selection cancelled'})
                                                return
                                             end                                          
                                             return self:_install_correct_tool(event)
                                          end)
end

function StructureEraser:_switch_tool(tool_name, hover_entity)
   self:_remove_tool_selection_capture()

   if self._current_tool then
      self._current_tool:destroy()
      self._current_tool = nil
   end

   self._current_tool_name = tool_name
   if tool_name == 'erase_floor' then
      self._current_tool = self:_start_erase_floor_tool()
   elseif tool_name == 'erase_fixture' then
      self._current_tool = self:_start_erase_fixture_tool(hover_entity, hover_entity)
   end
   
   self:_add_tool_selection_capture()
end

function StructureEraser:_start_erase_floor_tool()
   return stonehearth.selection:select_xz_region()
      :require_unblocked(false)
      :select_front_brick(false)
      :set_find_support_filter(stonehearth.selection.make_delete_floor_xz_region_support_filter())
      :set_cursor('stonehearth:cursors:create_floor')
      :use_manual_marquee(function(selector, box)
            local n = _radiant.client.create_voxel_node(H3DRootNode, Region3(box), 'materials/blueprint.material.json', Point3(0.5, 0, 0.5))
            n:set_polygon_offset(-5, -5)
            return n
         end)
      :done(function(selector, box)
            _radiant.call_obj(self._build_service, 'erase_floor_command', box)
               :done(function(r)
                     self:resolve(r)
                  end)
               :fail(function(r)
                     self:reject(r)
                  end)
         end)
      :go()
end

function StructureEraser:_start_erase_fixture_tool(hover_entity)
   local function erase_fixture_tool(event)
      if event and event:up(1) and not event.dragging then
         local s = _radiant.client.query_scene(event.x, event.y)
         for result in s:each_result() do
            if result.entity == hover_entity then
               _radiant.call_obj(self._build_service, 'erase_fixture_command', hover_entity)
                  :done(function(r)
                        self:resolve(r)
                     end)
                  :fail(function(r)
                        self:reject(r)
                     end)
               return
            end
         end
      end
   end

   local cursor_obj = _radiant.client.set_cursor('stonehearth:cursors:erase_fixture')
   return stonehearth.input:capture_input()
                                 :on_mouse_event(function(event)
                                       erase_fixture_tool(event)
                                       return true
                                    end)
                                 :on_destroyed(function()
                                       cursor_obj:destroy()
                                    end)
end

function StructureEraser:destroy()
   self:reject({ error = 'tool cancelled' })
end

return StructureEraser
