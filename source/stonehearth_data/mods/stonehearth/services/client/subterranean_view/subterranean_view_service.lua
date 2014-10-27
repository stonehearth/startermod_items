local constants = require 'constants'
local TerrainType = require 'services.server.world_generation.terrain_type'
SubterraneanViewService = class()

local UNITY_PLUS_EPSILON = 1.000001

function SubterraneanViewService:initialize()
   local enable_mining = radiant.util.get_config('enable_mining', false)
   if not enable_mining then
      return
   end

   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      -- we have some floating point drift on the integer coordinates, not sure why yet
      self._sv.clip_height = constants.mining.Y_ALIGN * 4 * UNITY_PLUS_EPSILON
      self._sv.clip_enabled = false
      self._sv.initialized = true
   else
   end

   self._input_capture = stonehearth.input:capture_input()
   self._input_capture:on_keyboard_event(function(e)
         return self:_on_keyboard_event(e)
      end)

   self:_update()
end

function SubterraneanViewService:destroy()
   self._input_capture:destroy()
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
      h3dSetVerticalClipMax(self._sv.clip_height)
   else
      h3dClearVerticalClipMax()
   end
end

return SubterraneanViewService
