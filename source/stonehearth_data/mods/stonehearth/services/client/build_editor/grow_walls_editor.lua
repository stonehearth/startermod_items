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
            return building ~= nil and not self:_has_walls(building)
         end)
      :done(function(selector, entity)
            log:detail('box selected')
            if building then
               _radiant.call_obj(self._build_service, 'grow_walls_command', building, columns_uri, walls_uri)
            end
         end)
      :fail(function(selector)
            log:detail('failed to select building')
            selector:destroy()
            response:reject('failed')
         end)
      :always(function(selector)
            log:detail('selector called always')
            selector:destroy()
            response:resolve('done')
         end)
      :go()

   return self
end

function GrowWallsEditor:_has_walls(building)
   for _, child in building:get_component('entity_container'):each_child() do
     if child:get_component('stonehearth:wall') then
       return true
      end
   end
   return false
end

return GrowWallsEditor 