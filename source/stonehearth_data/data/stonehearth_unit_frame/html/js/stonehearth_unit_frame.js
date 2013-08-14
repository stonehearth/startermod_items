App.StonehearthUnitFrameView = App.View.extend({
	templateName: 'unitFrame',

   components: {
      "unit_info": {},
      "radiant:commands": {}
   },

   init: function() {
      this._super();

      var self = this;
      $(top).on("selection_changed.radiant", function (_, evt) {
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
         delay: { show: 400, hide: 0 }
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
      } else if (command.action == 'post') {         
         // $.post(command.post_uri, command.post_data);
         $.ajax({
            type: 'post',
            url: command.post_uri,
            contentType: 'application/json',
            data: JSON.stringify(command.post_data)
         });
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