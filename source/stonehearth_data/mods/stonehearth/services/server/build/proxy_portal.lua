local Proxy = require 'services.server.build.proxy'
local ProxyPortal = class(Proxy)

local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2

function ProxyPortal:__init(parent_proxy, arg1)
   self[Proxy]:__init(self, parent_proxy, arg1)

   local data = self:get_entity():get_component('stonehearth:portal'):get_data()
   if data and data.cutter then
      self._cutter_rgn = Region2()
      self._cutter_rgn:load(data.cutter)
   end
end

function ProxyPortal:get_cutter()
   return self._cutter_rgn
end

return ProxyPortal
