App.StonehearthSettingsView = App.View.extend({
   templateName: 'settings',

   modal: true,

   fromResToVal : function(shadowRes, shadowsEnabled) {
      if (!shadowsEnabled) {
         return 0;
      }
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

   },

   reloadableSettingDidChange : function() {
      this.applyConfig(false);
   },

   anySettingDidChange : function() {
      $('#applyButton').prop('disabled', false);
   },

   didInsertElement: function() {
      initIncrementButtons();

      var self = this;
      var reloadableCallback = function() {
         self.reloadableSettingDidChange();
      };
      var anythingChangedCallback = function() {
         self.anySettingDidChange();
      };

      $('#opt_numSamples').change(anythingChangedCallback);
      $('#opt_enableVsync').change(anythingChangedCallback);
      $('#opt_enableFullscreen').change(anythingChangedCallback);
      $('#opt_shadowRes').change(anythingChangedCallback);

      $('#opt_numSamples').change(reloadableCallback);
      $('#opt_shadowRes').change(reloadableCallback);

      radiant.call('radiant:get_config_options')
         .done(function(o) {
            self.oldConfig = {
               "shadows" : o.shadows.value,
               "vsync" : o.vsync.value,
               "shadow_res" : o.shadow_res.value,
               "fullscreen" : o.fullscreen.value,
               "msaa" : o.msaa.value,
               "draw_distance" : o.draw_distance.value
            };

            self.set('context.shadows_forbidden', !o.shadows.allowed);
            if (!o.shadows.allowed) {
               o.shadows.value = false;
            }
            self.set('context.shadows_enabled', o.shadows.value);

            self.set('context.shadow_res', self.fromResToVal(o.shadow_res.value, o.shadows.value))

            self.set('context.vsync_enabled', o.vsync.value);

            self.set('context.fullscreen_enabled', o.fullscreen.value);

            self.set('context.msaa_forbidden', !o.msaa.allowed);
            if (!o.msaa.allowed) {
               o.msaa.value = 0;
            }
            self.set('context.num_msaa_samples', o.msaa.value);

            self.set('context.draw_distance', o.draw_distance.value);

            $('#aaNumSlider').slider({
               value: self.get('context.num_msaa_samples'),
               min: 0,
               max: 4,
               step: 1,
               disabled: self.get('context.msaa_forbidden'),
               slide: function( event, ui ) {
                  anythingChangedCallback();
                  $('#aaNumDescription').html(i18n.t('stonehearth:settings_slider_' + ui.value));
               }
            }); 
            $('#aaNumDescription').html(i18n.t('stonehearth:settings_slider_' + self.get('context.num_msaa_samples')));

            $('#shadowResSlider').slider({
               value: self.get('context.shadow_res'),
               min: 0,
               max: 5,
               step: 1,
               disabled: self.get('context.shadows_forbidden'),
               slide: function( event, ui ) {
                  anythingChangedCallback();
                  $('#shadowResDescription').html(i18n.t('stonehearth:settings_slider_' + ui.value));
               }
            });
            $('#shadowResDescription').html(i18n.t('stonehearth:settings_slider_' + self.get('context.shadow_res')));

            $('#drawDistSlider').slider({
               value: self.get('context.draw_distance'),
               min: 500,
               max: 1000,
               step: 10,
               slide: function( event, ui ) {
                  anythingChangedCallback();
                  $('#drawDistDescription').html(ui.value);
               }
            });
            $('#drawDistDescription').html(self.get('context.draw_distance'));
         });

   },

   getUiConfig: function(persistConfig) {
      var newConfig = {
         "shadows" : $( "#shadowResSlider" ).slider( "value" ) > 0,
         "vsync" : $('#opt_enableVsync').is(':checked'),
         "fullscreen" : $('#opt_enableFullscreen').is(':checked'),
         "msaa" : $( "#aaNumSlider" ).slider( "value" ),
         "shadow_res" :  this.fromValToRes($( "#shadowResSlider" ).slider( "value" )),
         "persistConfig" : persistConfig,
         "draw_distance" : $( "#drawDistSlider" ).slider( "value" )
      };
      return newConfig;
   },

   applyConfig: function(persistConfig) {
      var newConfig = this.getUiConfig(persistConfig);
      radiant.call('radiant:set_config_options', newConfig);
   },

   dismiss: function() {
      radiant.call('radiant:set_config_options', this.oldConfig);
   },

   actions: {
      applySettings: function() {
         this.applyConfig(true);
         this.destroy();
      },

      close: function() {
         this.dismiss();
         this.destroy();
      },

      cancel: function() {
         this.dismiss();
         this.destroy();
      }
   },

});