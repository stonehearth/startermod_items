local Proxy = require 'services.build.proxy'
local ProxyPortal = class(Proxy)

local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2

function ProxyPortal:__init(parent_proxy, arg1)
   local component_name = 'stonehearth:portal'
   self[Proxy]:__init(self, parent_proxy, arg1, component_name)

   local data = self:get_entity():get_component_data(component_name)
   self._cutter_rgn = Region2()
   self._cutter_rgn:load(data.cutter)
end

function ProxyPortal:get_cutter()
   return self._cutter_rgn
end

return ProxyPortal
