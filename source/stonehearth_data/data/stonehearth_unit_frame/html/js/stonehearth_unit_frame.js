App.StonehearthUnitFrameView = App.View.extend({
	templateName: 'unitFrame',

   components: {
      "radiant:commands": {
         commands: []
      },
      "unit_info": {}
   },

   init: function() {
      this._super();

      var self = this;
      $(top).on("selection_changed.radiant", function (_, evt) {
         self._selected_entity = evt.data.selected_entity;
         if (self._selected_entity) {
            self.set('uri', self._selected_entity);
         } else {
            self.set('uri', null);
         }

      });
   },

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      $('#commandButtons').on('mouseover', '.commandButton', function(event){
         var target = event.target, $cmdBtn;
         if (target.tagName.toLowerCase() == 'img') {
            $cmdBtn = $(target).parent('.commandButton');
         } else {
            $cmdBtn = $(target);
         }
         $cmdBtn.tooltip({
            animation: true
         });
         $cmdBtn.tooltip('show');
      });
   },

   doCommand: function(command) {
      if (!command.enabled) {
         return;
      }
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
   }.observes('context'),

   /*
   //Example of listening on an array
   _onCommandChange: function() {
      console.log('commands changing!');
   }.observes('context.radiant:commands.commands'),
   */

});