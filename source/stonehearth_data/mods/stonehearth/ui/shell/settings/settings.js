App.StonehearthSettingsView = App.View.extend({
   templateName: 'settings',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   fromResToVal : function(shadowRes, shadowsEnabled) {
      if (!shadowsEnabled) {
         return 0;
      }
      return (Math.log(shadowRes) / Math.log(2)) - 8;
   },

   fromValToRes : function(shadowVal) {
      return Math.pow(2, shadowVal + 8);
   },

   fromSamplesToVal : function(msaaSamples, msaaEnabled) {
      if (!msaaEnabled || msaaSamples == 0) {
         return 0;
      }
      return (Math.log(msaaSamples) / Math.log(2));      
   },

   fromValToSamples : function(msaaVal) {
      if (msaaVal == 0) {
         return 0;
      }
      return Math.pow(2, msaaVal);
   },

   init : function() {
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

   didInsertElement : function() {
      var self = this;

      // Check to see if we're part of a modal dialog; if so, center us in it!
      if (self.$('#modalOverlay').length > 0) {
         self.$('#settings').position({
               my: 'center center',
               at: 'center center',
               of: '#modalOverlay'
            });
      }

      self.$('.tab').click(function() {
         var tabPage = $(this).attr('tabPage');

         self.$('.tabPage').hide();
         self.$('.tab').removeClass('active');
         $(this).addClass('active');

         self.$('#' + tabPage).show();
      });

      var reloadableCallback = function() {
         self.reloadableSettingDidChange();
      };

      self.$('#opt_numSamples').change(reloadableCallback);
      self.$('#opt_shadowRes').change(reloadableCallback);

      self.$('#applyButton').click(function() {
         self.applySettings();
      });

      self.$('#cancelButton').click(function() {
         self.cancel();
      })

      radiant.call('radiant:get_audio_config')
         .done(function(o) {
            //Move to done of other call
            $('#bgmMusicSlider').slider({
               value: o.bgm_volume * 100,  
               min: 0, 
               max: 100,
               step: 10,
               change: function(event, ui) {
                  self.$('#bgmMusicSliderDescription').html( self.$("#bgmMusicSlider" ).slider( "value" ) + '%');
               }
            });
            $('#bgmMusicSliderDescription').html( $("#bgmMusicSlider" ).slider( "value" ) + '%');

            //Move to done of other call
            $('#effectsSlider').slider({
               value: o.efx_volume * 100, 
               min: 0, 
               max: 100,
               step: 10,
               change: function(event, ui) {
                  self.$('#efxSliderDescription').html( self.$("#effectsSlider" ).slider( "value" ) + '%');
               }
            });
            $('#efxSliderDescription').html( $("#effectsSlider" ).slider( "value" ) + '%');
         });

      radiant.call('radiant:get_config_options')
         .done(function(o) {
            self.oldConfig = {
               "shadows" : o.shadows.value,
               "vsync" : o.vsync.value,
               "shadow_res" : o.shadow_res.value,
               "fullscreen" : o.fullscreen.value,
               "msaa" : o.msaa.value,
               "draw_distance" : o.draw_distance.value,
               "use_fast_hilite" : o.use_fast_hilite.value,
               "enable_ssao" : o.enable_ssao.value
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
            self.set('context.num_msaa_samples', self.fromSamplesToVal(o.msaa.value, o.msaa.allowed));

            self.set('context.draw_distance', o.draw_distance.value);

            self.set('context.use_fast_hilite', o.use_fast_hilite.value);

            self.set('context.ssao_forbidden', !o.enable_ssao.allowed);
            if (!o.enable_ssao.allowed) {
               o.enable_ssao.value = false;
            }
            self.set('context.enable_ssao', o.enable_ssao.value);

            $('#gfxCardString').html(i18n.t('stonehearth:settings_gfx_cardinfo', {
               "gpuRenderer": o.gfx_card_renderer, 
               "gpuDriver": o.gfx_card_driver
            }));

            $('#aaNumSlider').slider({
               value: self.get('context.num_msaa_samples'),
               min: 0,
               max: 4,
               step: 1,
               disabled: self.get('context.msaa_forbidden'),
               slide: function( event, ui ) {
                  $('#aaNumDescription').html(i18n.t('stonehearth:settings_aa_slider_' + ui.value));
               }
            }); 
            $('#aaNumDescription').html(i18n.t('stonehearth:settings_aa_slider_' + self.get('context.num_msaa_samples')));

            $('#shadowResSlider').slider({
               value: self.get('context.shadow_res'),
               min: 0,
               max: 5,
               step: 1,
               disabled: self.get('context.shadows_forbidden'),
               slide: function( event, ui ) {
                  $('#shadowResDescription').html(i18n.t('stonehearth:settings_shadow_' + ui.value));
               }
            });
            $('#shadowResDescription').html(i18n.t('stonehearth:settings_shadow_' + self.get('context.shadow_res')));

            $('#drawDistSlider').slider({
               value: self.get('context.draw_distance'),
               min: 500,
               max: 1000,
               step: 10,
               slide: function( event, ui ) {
                  $('#drawDistDescription').html(ui.value);
               }
            });
            $('#drawDistDescription').html(self.get('context.draw_distance'));

         });

   },

   getGraphicsConfig: function(persistConfig) {
      var newConfig = {
         "shadows" : $( "#shadowResSlider" ).slider( "value" ) > 0,
         "vsync" : $('#opt_enableVsync').is(':checked'),
         "fullscreen" : $('#opt_enableFullscreen').is(':checked'),
         "msaa" : this.fromValToSamples($( "#aaNumSlider" ).slider( "value" )),
         "shadow_res" :  this.fromValToRes($( "#shadowResSlider" ).slider( "value" )),
         "persistConfig" : persistConfig,
         "draw_distance" : $( "#drawDistSlider" ).slider( "value" ),
         "use_fast_hilite" : $('#opt_useFastHilite').is(':checked'),
         "enable_ssao" : $('#opt_enableSsao').is(':checked'),
      };
      return newConfig;
   },

   getAudioConfig: function() {
      var newVolumeConfig = {
         "bgm_volume" : this.$( "#bgmMusicSlider" ).slider( "value" ) / 100,
         "efx_volume" : this.$( "#effectsSlider" ).slider( "value" ) / 100
      };

      return newVolumeConfig;
   },

   applyConfig: function(persistConfig) {
      var newConfig = this.getGraphicsConfig(persistConfig);
      radiant.call('radiant:set_config_options', newConfig);

      var audioConfig = this.getAudioConfig();
      radiant.call('radiant:set_audio_config', audioConfig);
   },

   dismiss: function() {
      radiant.call('radiant:set_config_options', this.oldConfig);
   },

   applySettings: function() {
      this.applyConfig(true);
      this.destroy();
   },

   cancel: function() {
      this.dismiss();
      this.destroy();
   },

});