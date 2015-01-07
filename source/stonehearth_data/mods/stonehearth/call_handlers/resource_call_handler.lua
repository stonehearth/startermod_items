local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local priorities = require('constants').priorities.worker_task
local farming_service = stonehearth.farming

local ResourceCallHandler = class()


function ResourceCallHandler:box_harvest_resources(session, response)
   stonehearth.selection:select_xz_region()
      :require_supported(true)
      :use_outline_marquee(Color4(0, 255, 0, 32), Color4(0, 255, 0, 255))
      :set_cursor('stonehearth:cursors:harvest')
      :set_find_support_filter(function(result)
            if result.entity:get_component('terrain') then
               return true
            end
            return stonehearth.selection.FILTER_IGNORE
         end)
      :done(function(selector, box)
            _radiant.call('stonehearth:server_box_harvest_resources', box)
            response:resolve(true)
         end)
      :fail(function(selector)            
            response:reject('no region')
         end)
      :go()
end


function ResourceCallHandler:server_box_harvest_resources(session, response, box)
   local cube = Cube3(Point3(box.min.x, box.min.y, box.min.z),
                      Point3(box.max.x, box.max.y, box.max.z))

   local entities = radiant.terrain.get_entities_in_cube(cube)
   
   for _, entity in pairs(entities) do
      self:harvest_entity(session, response, entity)
   end
end

--Call this one if you want to harvest as renewably as possible
function ResourceCallHandler:harvest_entity(session, response, entity)
   local town = stonehearth.town:get_town(session.player_id)

   if entity:get_component('stonehearth:renewable_resource_node') then
      town:harvest_renewable_resource_node(entity)
   elseif entity:get_component('stonehearth:resource_node') then
      town:harvest_resource_node(entity)
   elseif entity:get_component('stonehearth:crop') then
      town:harvest_crop(entity, '/stonehearth/data/effects/chop_overlay_effect')
   end
end

--Call this one if you want to destroy the entity permanently
function ResourceCallHandler:permanently_harvest_entity(session, response, entity)
   local town = stonehearth.town:get_town(session.player_id)

   if entity:get_component('stonehearth:resource_node') then
      town:harvest_resource_node(entity)
   end
end

function ResourceCallHandler:shear_sheep(session, response, sheep)
   asset(false, 'shear_sheep was broken, so i deleted it =)  -tony')
end


return ResourceCallHandler
