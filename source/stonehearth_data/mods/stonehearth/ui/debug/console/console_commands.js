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

   radiant.console.register('select', {
      call: function(cmdobj, fn, args) {
         var entity = 'object://game/' + args[0]
         return radiant.call('radiant:client:select_entity', entity);
      },
   });
});
