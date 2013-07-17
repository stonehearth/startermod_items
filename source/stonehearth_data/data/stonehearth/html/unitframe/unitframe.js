App.UnitFrameView = App.View.extend({
	templateName: 'unitframe',
	components: ['unit_info', 'radiant:commands'],

   init: function() {
      this._super();

      var self = this;
      $(top).on("radiant.events.selection_changed", function (_, evt) {
         self._selected_entity = evt.data.selected_entity;
         if (self._selected_entity) {
            self.set('uri', self._selected_entity);
            //$('#unitframe').show()
         } else {
            //$('#unitframe').hide()
            self.set('uri', null);
         }

      });
   },

   didInsertElement: function() {
      $('.commandButton').tooltip({
         delay: { show: 800, hide: 0 }
         });
   },

   doCommand: function(command) {
      if (command.action == 'fire_event') {
         // xxx: error checking would be nice!!
         var e = {
            entity : this._selected_entity,
            event_data : command.event_data
         };
         $(top).trigger(command.event_name, e);
      }
   },

   _setVisibility: function() {
      if (this.get('context')) {
         $('#unitframe').show();
      } else {
         $('#unitframe').hide();
      }

   }.observes('context')
});