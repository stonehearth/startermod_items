local ProxyFabrication = require 'services.server.build.proxy_fabrication'
local ProxyFloor = class(ProxyFabrication)

-- this is the component which manages the fabricator entity.
function ProxyFloor:__init(parent_proxy, arg1)
   self[ProxyFabrication]:__init(self, parent_proxy, arg1)
end

return ProxyFloor
