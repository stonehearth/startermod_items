local portrait_renderer = stonehearth.portrait_renderer

local PortraitCallHandler = class()
local log = radiant.log.create_logger('portrait_renderer')

function PortraitCallHandler:render_portrait(session, response, json)
   log:error('portrait callback')
   portrait_renderer:render_scene(session, response, json)
end

return PortraitCallHandler
