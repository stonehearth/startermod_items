App.UnitFrameView = App.View.extend({
	templateName: 'unitframe',
	components: ['unit_info', 'radiant:commands'],

   init: function() {
      this._super();

      var self = this;
      $(top).on("radiant.events.selection_changed", function (_, evt) {
         var uri = evt.data.selected_entity;
         if (uri) {
            self.set('uri', uri);
         }
      });
   },

   didInsertElement: function() {
      $('.commandButton').tooltip({
         delay: { show: 800, hide: 0 }
         });
   },

   doCommand: function(handler) {
      console.log(handler);
   }
});