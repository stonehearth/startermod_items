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
            self.set('context.shadows_forbidden', !o.shadows.allowed);
            if (!o.shadows.allowed) {
               o.shadows.value = false;
            }
            self.set('context.shadows_enabled', o.shadows.value);

            self.set('context.vsync_enabled', o.vsync.value);

            self.set('context.fullscreen_enabled', o.fullscreen.value);

            self.set('context.msaa_forbidden', !o.msaa.allowed);
            if (!o.msaa.allowed) {
               o.msaa.value = 0;
            }
            self.set('context.num_msaa_samples', o.msaa.value);
         });
   },

   didInsertElement: function() {
      initIncrementButtons();
   },

   actions: {
      applySettings: function() {
         var newConfig = {
            "shadows" : $('#opt_enableShadows').is(':checked'),
            "enable_vsync" : $('#opt_enableVsync').is(':checked'),
            "enable_fullscreen" : $('#opt_enableFullscreen').is(':checked'),
            "msaa" : parseInt($('#opt_numSamples').val())
         };

         radiant.call('radiant:set_config_options', newConfig);

         this.destroy();
      },

      close: function() {
         this.destroy();
      }
   },

});