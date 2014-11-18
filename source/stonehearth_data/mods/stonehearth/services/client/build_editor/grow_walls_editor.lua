local build_util = require 'lib.build_util'
local constants = require('constants').construction

local GrowWallsEditor = class()
local log = radiant.log.create_logger('build_editor.grow_walls')

-- this is the component which manages the fabricator entity.
function GrowWallsEditor:__init(build_service)
   log:debug('created')

   self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
end

function GrowWallsEditor:destroy()
   log:debug('destroyed')
end

function GrowWallsEditor:go(response, columns_uri, walls_uri)
   local building

   log:detail('running')
   stonehearth.selection:select_entity_tool()
      :set_cursor('stonehearth:cursors:grow_walls')
      :set_filter_fn(function(entity)
            if build_util.is_footprint(entity) then
               return stonehearth.selection.FILTER_IGNORE
            end
            building = build_util.get_building_for(entity)
            return building ~= nil and not self:is_road(building) and not build_util.has_walls(building)
         end)
      :done(function(selector, entity)
            log:detail('box selected')
            if building then
               _radiant.call_obj(self._build_service, 'grow_walls_command', building, columns_uri, walls_uri)
            end
            response:resolve('done')
         end)
      :fail(function(selector)
            log:detail('failed to select building')
            response:reject('failed')
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


return GrowWallsEditor 