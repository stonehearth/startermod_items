local Region2 = _radiant.csg.Region2

local PortalComponent = class()
local log = radiant.log.create_logger('build')

function PortalComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   if not self._sv.region2 then
      self._portal_region = Region2()
      self._portal_region:load(json.cutter)
   end
end

function PortalComponent:get_portal_region()
   return self._portal_region
end

return PortalComponent
