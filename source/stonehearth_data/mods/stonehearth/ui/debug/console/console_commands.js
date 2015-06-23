$(document).ready(function(){
   var selected;

   $(top).on("radiant_selection_changed.unit_frame", function (_, data) {
      selected = data.selected_entity;
   });
   
   // wire up a generic handler to call any registered route.
   radiant.console.register('help', {
      call : function(cmdobj, fn, args) {
         var commands = radiant.console.getCommands();
         var commands = "<p>Available commands:</p>" + commands;
         cmdobj.setContent(commands);
         return;
      },
      description : "Prints out all the registered console commands."
   });

   // wire up a generic handler to call any registered route.
   radiant.console.register('call', {
      call : function(cmdobj, fn, args) {
         var cmd = args[0];
         var cmdargs = args.slice(1);
         return radiant.callv(cmd, cmdargs).deferred
      },
      description : "A generic handler to call any registered route."
   });

   radiant.console.register('show_pathfinder_time', {
      graphValueKey: 'total',
      call: function() {
         return radiant.call('radiant:show_pathfinder_time');
      },
   });

   radiant.console.register('destroy', {
      call: function(cmdobjs, fn, args) {
         var entity;
         if (args.length > 0) {
            entity = 'object://game/' + args[0];
         } else {
            entity = selected;
         }
         return radiant.call('stonehearth:destroy_entity', entity);
      },
      description : "Destroy an entity. Arg 0 is id of the entity. If no argument is provided, destroys the currently selected entity. Usage: destroy 12345"
   });

   radiant.console.register('kill', {
      call: function(cmdobjs, fn, args) {
         var entity;
         if (args.length > 0) {
            entity = 'object://game/' + args[0];
         } else {
            entity = selected;
         }
         return radiant.call('stonehearth:kill_entity', entity);
      },
      description : "Kill an entity. Arg 0 is id of the entity. If no argument is provided, kills the currently selected entity. Usage: kill 12345"
   });

   // Usage: select 12345  # enity id
   radiant.console.register('select', {
      call: function(cmdobj, fn, args) {
         var entity = 'object://game/' + args[0]
         return radiant.call('radiant:client:select_entity', entity);
      },
      description : "Selects the entity with id = Arg 0. Usage: select 12345"
   });

   // Usage: get_config foo.bar.baz
   radiant.console.register('get_config', {
      call: function(cmdobj, fn, args) {
         var key = args[0]
         return radiant.call('radiant:get_config', key);
      },
      description : "Usage: get_config foo.bar.baz"
   });

   // Usage: set_config foo.bar.baz { value = 1 }
   radiant.console.register('set_config', {
      call: function(cmdobj, fn, args) {
         var key = args[0];
         var value = JSON.parse(args[1]);
         return radiant.call('radiant:set_config', key, value);
      },
      description : "Usage: set_config foo.bar.baz { value = 1 }"
   });

   radiant.console.register('ib', {
      call: function(cmdobj, fn, args) {
         var building = args[0] || selected;
         return radiant.call_obj('stonehearth.build', 'instabuild_command', building);
      },
      description : "Instantly builds the selected building, or arg 0. Usage: ib object://game/12345"
   });

   radiant.console.register('get_cost', {
      call: function(cmdobj, fn, args) {
         var building = args[0] || selected;
         return App.stonehearthClient.getCost(building);
      },
      description : "Get the cost of the selected building, or arg 0. Usage: get_cost object://game/12345"
   });

   radiant.console.register('query_pf', {
      call: function(cmdobj, fn, args) {
         return radiant.call_obj('stonehearth.selection', 'query_pathfinder_command');
      },
      description : "Runs the query pathfinder command. No arguments."
   });

   radiant.console.register('spawn_scenario', {
      call: function(cmdobj, fn, args) {
         var scenario_uri = args[0];
         return App.stonehearthClient.spawnScenario(scenario_uri);
      },
      description : "Spawns the specified scenario uri. Usage: spawn_scenario stonehearth:quests:collect_starting_resources"
   });

   radiant.console.register('set_time', {
      call: function(cmdobj, fn, args) {
         var time = JSON.parse(args[0]);
         return App.stonehearthClient.setTime(time);
      },
      description : "Sets the time with options passed in through Arg 0 (Note you can't have any spaces and you have to include the quotes so is valid JSON). Usage: set_time {\"hour\":12,\"minute\":0}"
   });

   radiant.console.register('world_seed', {
      call: function(cmdobj, fn, args) {
         return radiant.call_obj('stonehearth.world_generation', 'get_world_seed_command');
      },
      description : "Returns the world seed of the current world. Usage: world_seed"
   });
});
