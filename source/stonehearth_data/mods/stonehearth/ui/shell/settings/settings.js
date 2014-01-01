App.StonehearthSettingsView = App.View.extend({
   templateName: 'settings',

   modal: true,

   fromResToVal : function(shadowRes) {
      return (Math.log(shadowRes) / Math.log(2)) - 8;
   },

   fromValToRes : function(shadowVal) {
      return Math.pow(2, shadowVal + 8);
   },

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

            self.set('context.shadow_res', self.fromResToVal(o.shadow_res.value))

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
            "vsync" : $('#opt_enableVsync').is(':checked'),
            "fullscreen" : $('#opt_enableFullscreen').is(':checked'),
            "msaa" : parseInt($('#opt_numSamples').val()),
            "shadow_res" : this.fromValToRes(parseInt($('#opt_shadowRes').val()))
         };

         radiant.call('radiant:set_config_options', newConfig);

         this.destroy();
      },

      close: function() {
         this.destroy();
      }
   },

});