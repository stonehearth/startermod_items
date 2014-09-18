local AutotestDispatcher = class()
AutotestDispatcher.name = 'run test'
AutotestDispatcher.does = 'stonehearth:top'
AutotestDispatcher.args = {}
AutotestDispatcher.version = 2
AutotestDispatcher.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(AutotestDispatcher)
         :execute('stonehearth_autotest:run_test')
