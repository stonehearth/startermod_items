local TestObjectMonitorStartThinking = class()

local Entity = _radiant.om.Entity

TestObjectMonitorStartThinking.name = 'object monitor test'
TestObjectMonitorStartThinking.does = 'stonehearth_autotest:test_object_monitor_start_thinking'
TestObjectMonitorStartThinking.args = {
   monitor = Entity,       -- the thing to monitor
   autotest = 'table',     -- the autotest instance
   delay_ms = {
      type = 'number',    -- how long to wait
      default = 0
   }
}
TestObjectMonitorStartThinking.version = 2
TestObjectMonitorStartThinking.priority = 1

function TestObjectMonitorStartThinking:start_thinking(ai, entity, args)
   radiant.events.trigger_async(entity, 'start_thinking')
   if args.delay_ms == 0 then
      args.autotest:fail('timer expired!')
   end

   self._timer = radiant.set_realtime_timer(args.delay_ms, function()
         args.autotest:fail('timer expired!')
      end)
end

function TestObjectMonitorStartThinking:stop_thinking(ai, entity, args)
   if self._timer then
      self._timer:destroy()
      self._timer = nil
      args.autotest:success()
   end
end

return TestObjectMonitorStartThinking
