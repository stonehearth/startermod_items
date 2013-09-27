local WorldGenerator = require 'services.world_generation.world_generator'
local WorldGenerationService = class()

function WorldGenerationService:__init()
end

function WorldGenerationService:create_world(use_async)
   local wg = WorldGenerator()
   wg:create_world(use_async)
   return wg
end

return WorldGenerationService()

