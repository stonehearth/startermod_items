App.StonehearthUnitFrameView = App.View.extend({
	templateName: 'unitFrame',

   components: {
      "stonehearth:commands": {
         commands: []
      },
      "unit_info": {}
   },

   init: function() {
      this._super();

      var self = this;
      $(top).on("selection_changed.radiant", function (_, data) {
         self._selected_entity = data.selected_entity;
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

   _setVisibility: function() {
      if (this.get('context')) {
         $('#unitframe').show();
      } else {
         $('#unitframe').hide();
      }
   }.observes('context')

});

App.StonehearthCommandButtonView = App.View.extend({
   oneBasedIndex: function() {
      return this.contentIndex + 1;
   }.property(),

   actions: {
      doCommand: function(command) {
         if (!command.enabled) {
            return;
         }
         if (command.action == 'fire_event') {
            // xxx: error checking would be nice!!
            var e = {
               entity : this.get("parentView.uri"),
               event_data : command.event_data
            };
            $(top).trigger(command.event_name, e);
         } else if (command.action == 'call') {
            if (command.object) {
               radiant.call_objv(command.object, command['function'], command.args)
            } else {
               radiant.callv(command['function'], command.args)
            }
         } else {
            throw "unknown command.action " + command.action
         }
      }
   }
});