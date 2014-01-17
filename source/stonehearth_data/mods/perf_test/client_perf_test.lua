-- xxx, this sucks. force a load of the client side serivces like the camera
radiant.mods.load('stonehearth')

local Vec3 = _radiant.csg.Point3f
local ClientPerfTest = class()
local camera = radiant.mods.load('stonehearth').camera

function ClientPerfTest:__init()
  self._frame_trace = nil
  self._running_time = 0
  self._last_now = 0
  -- Make sure we draw the world.
  _radiant.call('perf_test:get_world_generation_done'):done(function(response)
      if not response.result then
        return
      end
      _radiant.call('radiant:set_draw_world', {["draw_world"] = true})
      _radiant.renderer.enable_perf_logging(true)

      self._frame_trace = _radiant.client.trace_render_frame()
      self._frame_trace:on_frame_start('perf', function(now, interpolate)
          local delta = now - self._last_now
          if delta > 100 then
            self._running_time = self._running_time + 100
            self._last_now = now
            --HACKHACKHACKHACK
            camera._next_position = Vec3(math.cos(now / 5000.0) * 700, 150.0 + (math.sin(now / 1000.0) * 20), math.sin(now / 2500.0) * 700)
            _radiant.renderer.camera.look_at(Vec3(0.0, 0.0, 0.0))
          end

          if self._running_time > 30000 then
            _radiant.renderer.enable_perf_logging(false)
            --need a way to kill this
          end

          return true
        end)
    end)
end

return ClientPerfTest()