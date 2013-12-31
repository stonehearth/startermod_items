App.StonehearthSettingsView = App.View.extend({
   templateName: 'settings',

   modal: true,

   init: function() {
      this._super();
      var self = this;
      $(top).on('keyup keydown', function(e){
         if (e.keyCode == 27) {
            //If escape, close window
            self.destroy();
         }
      });

      radiant.call('radiant:get_config_options')
         .done(function(o) {
            self.set('context.shadows_enabled', !!o.shadows.value);
            self.set('context.shadows_forbidden', !o.shadows.allowed);
         });
   },

   actions: {
      applySettings: function() {
         var newConfig = {
            "shadows" : $('#opt_enableShadows').is(':checked')
         };

         radiant.call('radiant:set_config_options', newConfig);

         this.destroy();
      },

      close: function() {
         this.destroy();
      }
   },

});