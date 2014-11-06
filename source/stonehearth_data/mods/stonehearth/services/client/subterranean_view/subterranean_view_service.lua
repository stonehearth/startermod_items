local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local constants = require 'constants'
local TerrainType = require 'services.server.world_generation.terrain_type'

SubterraneanViewService = class()

local EPSILON = 0.000001
local MAX_CLIP_HEIGHT = 1000000000

function SubterraneanViewService:initialize()
   local enable_mining = radiant.util.get_config('enable_mining', false)
   if not enable_mining then
      return
   end

   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.clip_enabled = false
      -- we have some floating point drift on the integer coordinates, not sure why yet
      self._sv.clip_height = 25
      self._sv.initialized = true
   else
   end

   self._initialize_listener = radiant.events.listen(radiant, 'stonehearth:gameloop', function()
         local root_entity = _radiant.client.get_object(1)
         if root_entity and root_entity:is_valid() then
            -- still broken, need to wait for the renderterrain object to be created
            --self:_update()
            self:_destroy_initialize_listener()
         end
      end)
end

function SubterraneanViewService:destroy()
   self:_destroy_initialize_listener()
   self._input_capture:destroy()
end

function SubterraneanViewService:_destroy_initialize_listener()
   if self._initialize_listener then
      self._initialize_listener:destroy()
      self._initialize_listener = nil
   end
end

function SubterraneanViewService:set_clip_enabled(enabled)
   self._sv.clip_enabled = enabled
   self.__saved_variables:mark_changed()
   self:_update()
end

function SubterraneanViewService:set_clip_height(height)
   self._sv.clip_height = height
   self.__saved_variables:mark_changed()
   self:_update()
end

function SubterraneanViewService:move_clip_height_up()
   self:set_clip_height(self._sv.clip_height + constants.mining.Y_CELL_SIZE);
end

function SubterraneanViewService:move_clip_height_down()
   self:set_clip_height(self._sv.clip_height - constants.mining.Y_CELL_SIZE);
end

function SubterraneanViewService:set_clip_enabled_command(sessions, response, enabled)
   self:set_clip_enabled(enabled)
   return {}
end

function SubterraneanViewService:move_clip_height_up_command(session, response)
   self:move_clip_height_up()
   return {}
end

function SubterraneanViewService:move_clip_height_down_command(session, response)
   self:move_clip_height_down()
   return {}
end

function SubterraneanViewService:_update()
   if self._sv.clip_enabled then
      -- tweak the clip_height for some feathering in the renderer
      local tweaked_clip_height = self._sv.clip_height * (1+10*EPSILON)
      -- -1 to remove the ceiling
      _radiant.renderer.set_clip_height(tweaked_clip_height-1)
      h3dSetVerticalClipMax(tweaked_clip_height)
   else
      _radiant.renderer.set_clip_height(MAX_CLIP_HEIGHT)
      h3dClearVerticalClipMax()
   end
end

return SubterraneanViewService
