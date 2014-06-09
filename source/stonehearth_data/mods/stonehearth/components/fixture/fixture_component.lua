local Rect2 = _radiant.csg.Rect2
local FixtureComponent = class()

local MARGIN = { 'top', 'bottom', 'left', 'right' }

function FixtureComponent:initialize(entity, json)
   self._json = json
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.bounds = Rect2()
      self._sv.bounds:load(json.bounds)
      self._sv.margin = {}
      for _, k in pairs(MARGIN) do
         self._sv.margin[k] = (self._json.margin and self._json.margin[k]) or 0
      end     
      self.__saved_variables:mark_changed()
   end
end

-- ok to be nil
function FixtureComponent:get_valign()
   return self._json.valign
end

function FixtureComponent:get_margin()
   return self._sv.margin
end

function FixtureComponent:get_bounds()
   return self._sv.bounds
end

function FixtureComponent:get_cursor()
   return self._json.cursor
end

return FixtureComponent
