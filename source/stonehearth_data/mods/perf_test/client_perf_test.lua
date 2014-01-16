-- xxx, this sucks. force a load of the client side serivces like the camera
radiant.mods.load('stonehearth')

local Vec3 = _radiant.csg.Point3f
local ClientPerfTest = class()

function ClientPerfTest:__init()
  self._frame_trace = nil
  self._last_now = 0
    -- Make sure we draw the world.
  _radiant.call('perf_test:get_world_generation_done'):done(function(response)
      _radiant.call('radiant:set_draw_world', {["draw_world"] = true})
      _radiant.renderer.enable_perf_logging(true)

      self._frame_trace = _radiant.client.trace_render_frame()
      self._frame_trace:on_frame_start('perf', function(now, interpolate)
          local delta = now - self._last_now
          if delta > 500 then
            self._last_now = now
            _radiant.renderer.camera.set_position(Vec3(math.cos(now / 1000000.0) * 500, math.sin(now / 1000000.0) * 500, 50.0 + (math.sin(now / 1000000.0) * 20)))
            --_radiant.renderer.camera.look_at(Vec3(0.0, 0.0, 0.0))
          end
          return true
        end)
    end)
end

return ClientPerfTest()