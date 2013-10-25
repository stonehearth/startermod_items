local Proxy = require 'services.build.proxy'
local ProxyContainer = class(Proxy)

-- this is the component which manages the fabricator entity.
function ProxyContainer:__init(parent_proxy)
   self[Proxy]:__init(self, parent_proxy, nil, nil)
end

return ProxyContainer
