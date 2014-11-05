-- xxx, this sucks. force a load of the client side serivces like the camera

local Vec3 = _radiant.csg.Point3
local ClientPerfTest = class()

function ClientPerfTest:__init()
  radiant.events.listen(radiant, 'radiant:modules_loaded', self, self.on_loaded)
end

function ClientPerfTest:on_loaded()
  local camera = stonehearth.camera
  self._frame_trace = nil
  self._running_time = 0
  self._last_now = 0
  
  -- Make sure we draw the world.
  _radiant.call('perf_test:get_world_generation_done'):done(function(response)
      if not response.result then
        return
      end
      _radiant.call('radiant:set_draw_world', true)
      _radiant.renderer.enable_perf_logging(true)

      self._frame_trace = _radiant.client.trace_render_frame()
      self._frame_trace:on_frame_start('perf', function(now, interpolate)
          local delta = now - self._last_now
          if delta > 100 then
            if self._last_now ~= 0 then
              self._running_time = self._running_time + delta
            end
            self._last_now = now
            --HACKHACKHACKHACK
            camera._next_position = Vec3(math.cos(now / 5000.0) * 700, 150.0 + (math.sin(now / 1000.0) * 20), math.sin(now / 2500.0) * 700)
            _radiant.renderer.camera.look_at(Vec3(0.0, 0.0, 0.0))
          end

          if self._running_time > 30000 then
            _radiant.call('radiant:exit')
          end

          return true
        end)
    end)
end

return ClientPerfTest()
