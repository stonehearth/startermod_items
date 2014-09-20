local TestObjectMonitorRun = class()

local Entity = _radiant.om.Entity

TestObjectMonitorRun.name = 'object monitor test'
TestObjectMonitorRun.does = 'stonehearth_autotest:test_object_monitor_run'
TestObjectMonitorRun.args = {
   monitor = Entity,       -- the thing to monitor
   autotest = 'table',     -- the autotest instance
}
TestObjectMonitorRun.version = 2
TestObjectMonitorRun.priority = 1

function TestObjectMonitorRun:start_thinking(ai, entity, args)
   ai:set_think_output()
end

function TestObjectMonitorRun:run(ai, entity, args)
   radiant.events.trigger_async(entity, 'run')   
   while true do
      ai:suspend()
   end
end

function TestObjectMonitorRun:stop(ai, entity, args)
   args.autotest:success()
end

return TestObjectMonitorRun
