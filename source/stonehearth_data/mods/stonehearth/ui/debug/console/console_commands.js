$(document).ready(function(){
   var selected;

   $(top).on("radiant_selection_changed.unit_frame", function (_, data) {
      selected = data.selected_entity;
   });
   
   // wire up a generic handler to call any registered route.
   radiant.console.register('call', {
      call : function(cmdobj, fn, args) {
         var cmd = args[0];
         var cmdargs = args.slice(1);
         return radiant.callv(cmd, cmdargs).deferred
      },
   });

   radiant.console.register('show_pathfinder_time', {
      graphValueKey: 'total',
      call: function() {
         return radiant.call('radiant:show_pathfinder_time');
      },
   });

   radiant.console.register('destroy_entity', {
      call: function(cmdobjs, fn, args) {
         var entity = 'object://game/' + args[0]
         return radiant.call('stonehearth:destroy_entity', entity);
      }
   });

   radiant.console.register('kill_entity', {
      call: function(cmdobjs, fn, args) {
         var entity = 'object://game/' + args[0]
         return radiant.call('stonehearth:kill_entity', entity);
      }
   });

   // Usage: select 12345  # enity id
   radiant.console.register('select', {
      call: function(cmdobj, fn, args) {
         var entity = 'object://game/' + args[0]
         return radiant.call('radiant:client:select_entity', entity);
      },
   });

   // Usage: get_config foo.bar.baz
   radiant.console.register('get_config', {
      call: function(cmdobj, fn, args) {
         var key = args[0]
         return radiant.call('radiant:get_config', key);
      },
   });

   // Usage: set_config foo.bar.baz { value = 1 }
   radiant.console.register('set_config', {
      call: function(cmdobj, fn, args) {
         var key = args[0];
         var value = JSON.parse(args[1]);
         return radiant.call('radiant:set_config', key, value);
      },
   });

   radiant.console.register('instabuild', {
      call: function(cmdobj, fn, args) {
         var building = args[0];
         App.stonehearthClient.instabuild(building);
         return {};
      },
   });
});
