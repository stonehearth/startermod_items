local constants = require('constants').construction
local build_util = require 'lib.build_util'
local EntitySelector = require 'services.client.selection.entity_selector'
local XZRegionSelector = require 'services.client.selection.xz_region_selector'
local LocationSelector = require 'services.client.selection.location_selector'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('selection_service')

local SelectionService = class()

-- enumeration used by the filter functions of the various selection
-- services.
SelectionService.FILTER_IGNORE = 'ignore'

-- a filter function which looks for things which are solid.
--
SelectionService.find_supported_xz_region_filter = function (result)
   local entity = result.entity

   -- fast check for 'is terrain'
   if entity:get_id() == 1 then
      return true
   end

   -- solid regions are good if we're pointing at the top face
   if result.normal:to_int().y == 1 then
      local rcs = entity:get_component('region_collision_shape')
      if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
         return true
      end
   end

   -- otherwise, keep looking!
   return stonehearth.selection.FILTER_IGNORE
end

SelectionService.floor_xz_region_selector = function(result, raise_selector)
   -- stop if we've hit a piece of existing floor.  the call to  :allow_select_cursor(true)
   -- will save us most, but not all, of the time.  without both checks, we will occasionally
   -- stab through existing floor blueprints and hit the bottom of the terrain cut, creating
   -- another slab of floor below this one.
   --
   local entity = result.entity
   if entity then               
      local fc = entity:get_component('stonehearth:fabricator')
      if fc then
         local blueprint = build_util.get_blueprint_for(entity)
         local floor = blueprint:get_component('stonehearth:floor')
         if floor then
            if raise_selector and floor:get().category == constants.floor_category.CURB then
               result.brick = result.brick - Point3(0, 1, 0)
            end
            return true
         end
         -- if we blueprint which is not floor, bail
         return false
      end
   end
   
   -- fast check for 'is terrain'.  we can dig into the terrain, so leave the floor sunk.
   if entity:get_id() == 1 then
      return true
   end

   -- solid regions are good if we're pointing at the top face, but unsink the floor
   -- when doing so
   if result.normal:to_int().y == 1 then
      local rcs = entity:get_component('region_collision_shape')
      if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
         if raise_selector then
            result.brick = result.brick + result.normal
         end
         return true
      end
   end

   return false
end

SelectionService.make_edit_floor_xz_region_filter = function(result)
   return function(result)
      return SelectionService.floor_xz_region_selector(result, true)
   end
end

SelectionService.make_delete_floor_xz_region_filter = function(result)
   return function(result)
      return SelectionService.floor_xz_region_selector(result, false)
   end
end



local UNSELECTABLE_FLAG = _radiant.renderer.QueryFlags.UNSELECTABLE

   -- returns whether or not the zone can contain the specified entity
function SelectionService.designation_can_contain(entity)
   -- zones cannot be dragged around things that "take up space".  these things all
   -- have non-NONE collision regions
   local rcs = entity:get_component('region_collision_shape')
   if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
      return false
   end
   
   -- designations cannot contain other designation, either
   if radiant.entities.get_entity_data(entity, 'stonehearth:designation') then
      return false
   end
   return true
end

function SelectionService:initialize()
   self._all_tools = {}
   self._input_capture = stonehearth.input:capture_input()
                              :on_mouse_event(function(e)
                                   return self:_on_mouse_input(e)
                                 end)
end

function SelectionService:get_selected()
   return self._selected
end

function SelectionService:select_entity(entity)
   if entity and entity:get_component('terrain') then
      entity = nil
   end
   if entity == self._selected then
      return
   end

   local last_selected = self._selected
   _radiant.client.select_entity(entity)
   self._selected = entity

   if last_selected and last_selected:is_valid() then
      radiant.events.trigger(last_selected, 'stonehearth:selection_changed')
   end

   if entity and entity:is_valid() then
      radiant.events.trigger(entity, 'stonehearth:selection_changed')
   end
   radiant.events.trigger(radiant, 'stonehearth:selection_changed')
end

function SelectionService:select_xz_region()
   return XZRegionSelector()
end

function SelectionService:select_designation_region()
   return self:select_xz_region()
               :require_supported(true)
               :require_unblocked(true)
               :set_find_support_filter(function(result)
                     local entity = result.entity
                     -- make sure we draw zones atop either the terrain, something we've built, or
                     -- something that's solid
                     if entity:get_component('terrain') then
                        return true
                     end
                     if entity:get_component('stonehearth:construction_data') then
                        return true
                     end
                     local rcs = entity:get_component('region_collision_shape')
                     if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
                        return false
                     end
                     return stonehearth.selection.FILTER_IGNORE
                  end)
               :set_can_contain_entity_filter(function (entity)
                     return SelectionService.designation_can_contain(entity)
                  end)
end

function SelectionService:select_location()
   return LocationSelector()
end

function SelectionService:is_selectable(entity)
   if not entity or not entity:is_valid() then
      return false
   end

   local render_entity = _radiant.client.get_render_entity(entity)
   local selectable = render_entity and render_entity:has_query_flag(UNSELECTABLE_FLAG)
   return selectable
end

function SelectionService:set_selectable(entity, selectable)
   if not entity or not entity:is_valid() then
      return
   end

   local render_entity = _radiant.client.get_render_entity(entity)

   if render_entity then
      if selectable then
         render_entity:remove_query_flag(UNSELECTABLE_FLAG)
      else
         render_entity:add_query_flag(UNSELECTABLE_FLAG)
         if entity == self._selected then
            self:select_entity(nil)
         end
      end
   end
end

function SelectionService:select_entity_tool()
   return EntitySelector()
end

function SelectionService:register_tool(tool, enabled)
   self._all_tools[tool] = enabled and true or nil
end

function SelectionService:deactivate_all_tools()
   for tool, _ in pairs(self._all_tools) do
      tool:destroy()
   end
end

function SelectionService:_on_mouse_input(e)
   if e:up(1) and not e.dragging then
      local selected     
      local results = _radiant.client.query_scene(e.x, e.y)
      for r in results:each_result() do 
         local re = _radiant.client.get_render_entity(r.entity)
         if re and not re:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
            selected = r.entity
            break
         end
      end
      self:select_entity(selected)
      return true
   end

   return false
end

return SelectionService
