local build_util = require 'stonehearth.lib.build_util'

local Point3 = _radiant.csg.Point3

stonehearth_templates = {}

-- save the next template in the sequence.  call up to the server and ask it to
-- create the next guy in the series.
--
function stonehearth_templates.save_next_template()
   local render_trace

   _radiant.call_obj('stonehearth_templates.builder', 'build_next_template')
               :done(function(o)
                     -- yay!  building done!!  process it.
                     stonehearth_templates.process_building(o)
                  end)
               :fail(function(o)
                     radiant.log.write('', 0, 'error while processing templates (%s)', o)
                  end)
end

-- process the building.  basically just posing and taking a screenshot
--
function stonehearth_templates.process_building(o)
   stonehearth_templates.position_camera(o)
   stonehearth_templates.prepare_renderer(o)
   stonehearth_templates.take_screenshot(o)
end

-- this code is COMPLETELY bogus.  someone with more 3d math skills than I could
-- do a much better job!
--
function stonehearth_templates.position_camera(o)
   local bounds = build_util.get_building_bounds(o.building)
   local centroid = build_util.get_building_centroid(o.building)
   
   local max_d = math.max(bounds.max.x, math.max(bounds.max.y, bounds.max.z)) * 2
   local position = Point3()
   position.x = centroid.x + max_d * 3 / 4
   position.y = centroid.y + max_d * 3 / 4
   position.z = centroid.z + -max_d

   stonehearth.camera:set_position(position)
   stonehearth.camera:look_at(centroid)
end

-- prepare to render.  clear the selection.  unfortunately we can't clear the hilight,
-- so pray that the mouse isn't over the screen!
--
function stonehearth_templates.prepare_renderer(o)
   stonehearth.selection:select_entity()
end

-- take the screenshot.
--
function stonehearth_templates.take_screenshot(o)
   local render_count = 0
   local render_trace
   render_trace = _radiant.client.trace_render_frame()
                                    :on_frame_finished('save building', function(now, interpolate)
                                             -- we wait 5 frames to make sure all the crap we setup from
                                             -- stonehearth_templates.prepare_renderer has taken effect...
                                             render_count = render_count + 1
                                             if render_count > 5 then
                                                render_trace:destroy()
                                                o.screenshot = _radiant.client.snap_screenshot(o.name)
                                                radiant.log.write('', 0, 'took screenshot for building "%s"', o.name)

                                                -- woot!  screenshot saved.  now we can save the template.
                                                stonehearth_templates.save_template(o)
                                             end
                                       end)
end

-- call up to the server to save the template
--
function stonehearth_templates.save_template(o)
   local header = {
      name = o.name,
      screenshot = o.screenshot,
   }
   _radiant.call('stonehearth:save_building_template', o.building, header)
               :done(function()
                     -- all good!  move onto the next template in the chain
                     stonehearth_templates.save_next_template()
                  end)
               :fail(function(o)
                     radiant.log.write('', 0, 'error while saving template (%s)', o)                  
                  end)
end

radiant.events.listen(stonehearth_templates, 'radiant:new_game', function(args)
      _radiant.call('radiant:set_draw_world', true)
      stonehearth_templates.save_next_template()
   end)

return stonehearth_templates
