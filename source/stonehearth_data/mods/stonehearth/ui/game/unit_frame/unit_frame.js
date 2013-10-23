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
            self.show();
         } else {
            //self.set('uri', null);
            self.hide();
         }
      });
   },

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      $('#commandButtons')
         .off('mouseover', '.commandButton')
         .on('mouseover', '.commandButton', function(event){
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

   show: function() {
      $('#unitFrame').css({
         opacity: 1.0
      })      
   },

   hide: function() {
      $('#unitFrame').css({
         opacity: 0.0
      })      
   }

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