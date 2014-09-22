local Point3 = _radiant.csg.Point3

local ai_autotests = {}

-- setup an environment to run the object monitor autotests
local function setup_object_monitor_test(autotest, action_script, args)
   local e = autotest.env:create_entity(0, 0, 'stonehearth:oak_log')
   local ai = e:add_component('stonehearth:ai')
   
   ai:_initialize({
      actions = {
         '/stonehearth_autotest/ai/actions/autotest_dispatcher.lua',
         action_script,
      }
   }) 
   ai:start()

   radiant.entities.set_player_id(e, 'player_1')
   
   local activity_name = radiant.mods.load_script(action_script)().does;
   local monitor_entity = autotest.env:create_entity(1, 1)
   local task_group = ai:get_task_group('stonehearth_autotest:run_test')

   local task = task_group:create_task(activity_name, args)
                              :once()
   return e, task
end

-- destroy the object while thiniking.
function ai_autotests.destroy_during_thinking(autotest, test)   
   local script = '/stonehearth_autotest/ai/actions/test_object_monitor_start_thinking_action.lua'
   local monitor_entity = autotest.env:create_entity(1, 1)

   local e, task = setup_object_monitor_test(autotest, script, {
         monitor = monitor_entity,
         autotest = autotest,
         delay_ms = 1000,
      })

   radiant.events.listen_once(e, 'start_thinking', function()
         radiant.entities.destroy_entity(monitor_entity)
      end)
   task:start()
   autotest:sleep(5000)
   autotest:fail('ai failed to fail or succeed (really bad!)')
end


function ai_autotests.destroy_before_thinking(autotest, test)   
   local script = '/stonehearth_autotest/ai/actions/test_object_monitor_start_thinking_action.lua'
   local monitor_entity = autotest.env:create_entity(1, 1)

   radiant.entities.destroy_entity(monitor_entity)

   local e, task = setup_object_monitor_test(autotest, script, {
         monitor = monitor_entity,
         autotest = autotest
      })
   
   radiant.events.listen_once(e, 'start_thinking', function()
         autotest:fail('action with destroyed entity in args erronously started thinking')
      end)
   task:start()
   autotest:sleep(1000)
   autotest:success()
end

function ai_autotests.destroy_during_run(autotest, test)   
   local script = '/stonehearth_autotest/ai/actions/test_object_monitor_run_action.lua'
   local monitor_entity = autotest.env:create_entity(1, 1)

   local e, task = setup_object_monitor_test(autotest, script, {
         monitor = monitor_entity,
         autotest = autotest
      })
   
   radiant.events.listen_once(e, 'run', function()
         radiant.entities.destroy_entity(monitor_entity)
      end)
   task:start()
   autotest:sleep(1000)
   autotest:fail('failed to kill action when entity destroying during run')
end


return ai_autotests
