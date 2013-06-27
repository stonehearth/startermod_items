App.UnitFrameView = App.View.extend({
	templateName: 'unitframe',
	components: ['unit_info', 'radiant.abilities'],

   init: function() {
      this._super();

      var self = this;
      $(top).on("radiant.events.selection_changed", function (_, evt) {
         var uri = evt.data.selected_entity;
         console.log('selection changed to ' + uri);
         if (uri) {
            self.set('url', uri);
         }
      });
   }
});