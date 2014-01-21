local PerfTest = class()

function PerfTest:__init()
  _radiant.call('perf_test:set_is_running')
end

return PerfTest
