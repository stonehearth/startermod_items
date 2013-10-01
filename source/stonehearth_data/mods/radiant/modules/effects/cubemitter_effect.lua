local Effect = require 'modules.effects.effect'
local CubemitterEffect = class(Effect)

function CubemitterEffect:__init(info)  
   self[Effect]:__init(info)
end

function CubemitterEffect:update(now)
   return true
end

return CubemitterEffect
