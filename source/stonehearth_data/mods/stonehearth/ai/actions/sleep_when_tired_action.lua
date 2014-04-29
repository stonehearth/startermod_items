local SleepWhenTired = class()
SleepWhenTired.name = 'sleep when tired'
SleepWhenTired.does = 'stonehearth:sleep'
SleepWhenTired.args = {}
SleepWhenTired.version = 2
SleepWhenTired.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(SleepWhenTired)
            :execute('stonehearth:wait_for_attribute_above', { attribute = 'sleepiness', value = stonehearth.constants.sleep.MIN_SLEEPINESS })
            :execute("stonehearth:sleep_when_tired")
