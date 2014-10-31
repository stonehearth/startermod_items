local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local constants = require 'constants'
local TerrainType = require 'services.server.world_generation.terrain_type'

SubterraneanViewService = class()

local UNITY_PLUS_EPSILON = 1.000001
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
      self._sv.clip_height = 25 * UNITY_PLUS_EPSILON
      self._sv.initialized = true
   else
   end

   self._input_capture = stonehearth.input:capture_input()
   self._input_capture:on_keyboard_event(function(e)
         return self:_on_keyboard_event(e)
      end)

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

function SubterraneanViewService:_on_keyboard_event(e)
   if e.down then
      if e.key == _radiant.client.KeyboardInput.KEY_BACKSLASH then
         if self._sv.clip_enabled then
            self._sv.clip_enabled = false
         else
            self._sv.clip_enabled = true
         end
         self.__saved_variables:mark_changed()
         self:_update()
         return true
      end

      if self._sv.clip_enabled then
         if e.key == _radiant.client.KeyboardInput.KEY_LEFT_BRACKET then
            self._sv.clip_height = self._sv.clip_height - constants.mining.Y_ALIGN
            self.__saved_variables:mark_changed()
            self:_update()
            return true
         end
         if e.key == _radiant.client.KeyboardInput.KEY_RIGHT_BRACKET then
            self._sv.clip_height = self._sv.clip_height + constants.mining.Y_ALIGN
            self.__saved_variables:mark_changed()
            self:_update()
            return true
         end
      end
   end
   return false
end

function SubterraneanViewService:_update()
   if self._sv.clip_enabled then
      -- -1 to remove the ceiling
      _radiant.renderer.set_clip_height(self._sv.clip_height-1)
      h3dSetVerticalClipMax(self._sv.clip_height)
   else
      _radiant.renderer.set_clip_height(MAX_CLIP_HEIGHT)
      h3dClearVerticalClipMax()
   end
end

return SubterraneanViewService
