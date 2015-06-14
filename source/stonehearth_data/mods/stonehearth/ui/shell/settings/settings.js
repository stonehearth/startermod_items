// Message from Tom
// --------------------------
// This file is a crime against nature!!! Here's what's going on
//
// 1) Originally this was written in our usual, hacky way, going around ember
//    and having the view do all the work of grabbing data, etc.
// 2) In an attempt to restore sanity to the system, we're starting to do things
//    "the ember way" (starting 3/18/2015), meaning.
//    a) Ember auto-instantiates the view and controller when the 'openModal' action
//       is invoked in esc_menu.html
//    b) The controller SHOULD BE responsible for grabbing and managing the data.
//    c) The view SHOULD BE responsible for just displaying the data
//
// 3) Other controllers that need to access the settings can do dependency injecting by
//    adding "needs: ['settings']" to their definition. Yay!
//
// 4) Of course, neither 2a nor 2b are true for now. The settings controller grabs the
//    data, and the view grabs ANOTHER copy of the data and uses that copy.
//
// SO, Things to do when we get the time:
//
// 1) SettingsView should get its data from the controller, not make it's own calls
// 2) The settings template should bind to data directly in the controller where possible.
//    example: <div>{{controller.get.config_options.foo}}
// 3) SettingsView should make its own computed properties when necessary. Check gfxCardString
//    for an example of how to do this
// 4) SettingsController should have an action to change all the settings. The view should
//    call this.get('controller').send('setOptions', options)


App.SettingsController = Ember.Controller.extend({
   foo: 'foo',

   init: function() {
      console.log('settings controller!');
      var self = this;

      radiant.call('radiant:get_audio_config')
         .done(function(o) {
            self.set('audio_options', o);
         });

      radiant.call('radiant:get_config_options')
         .done(function(o) {
            if (!o.shadows.allowed) {
               o.shadows.value = false;
            }

            self.set('config_options', o)
         });

      radiant.call('radiant:get_config', 'force_32_bit')
         .done(function(o) {
            // if o.force_32_bit is undefined, set it to false
            var enabled = o.force_32_bit == true;
            self.set('force_32_bit', enabled)
         });

      radiant.call('radiant:get_config', 'collect_analytics')
         .done(function(o) {
            var enabled = o.collect_analytics != false;
            self.set('collect_analytics', enabled)
         });
   }
});

App.SettingsView = Ember.View.extend({
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,
   modal: true,

   gfxCardString: function() {
      var configOptions = this.get('controller.config_options');
      if (configOptions) {
         var str = i18n.t('stonehearth:settings_gfx_cardinfo', {
               "gpuRenderer": configOptions.gfx_card_renderer, 
               "gpuDriver": configOptions.gfx_card_driver
            });

         this.set('gfxCardString', str);         
      }
   }.observes('controller.config_options'),

   low_quality : function() {
      return !this.get('context.use_high_quality');
   }.property('context.use_high_quality'),

   qualityChanged : function() {
      if ($('#aaNumSlider').hasClass('ui-slider')) {
         $('#aaNumSlider').slider('option', 'disabled', this.get('low_quality'));
      }
   }.observes('low_quality'),

   foo : function() {
      console.log('yay!')
   }.observes('controller.audio_options'),

   fromResToVal : function(shadowRes, shadowsEnabled) {
      if (!shadowsEnabled) {
         return 0;
      }
      return shadowRes;
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
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:page_up' });
         var tabPage = $(this).attr('tabPage');

         self.$('.tabPage').hide();
         self.$('.tab').removeClass('active');
         $(this).addClass('active');

         self.$('#' + tabPage).show();
      });

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
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' });
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
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' });
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
               "shadow_quality" : o.shadow_quality.value,
               "max_lights" : o.max_lights.value,
               "fullscreen" : o.fullscreen.value,
               "msaa" : o.msaa.value,
               "draw_distance" : o.draw_distance.value,
               "use_high_quality" : o.use_high_quality.value,
               "enable_ssao" : o.enable_ssao.value
            };
            self._updateGfxTabPage(o);
         });

      radiant.call('radiant:get_config', 'force_32_bit')
         .done(function(o) {
            // if o.force_32_bit is undefined, set it to false
            var enabled = o.force_32_bit == true;
            self.set('context.force_32_bit', enabled);
         });
/*
      radiant.call('radiant:get_config', 'enable_lua_jit')
         .done(function(o) {
            // o.enable_lua_jit can be true, false, or undefined here.  if it's undefined or
            // true, we want to turn 64-bit mode on!
            var enabled = o.enable_lua_jit  != false;
            self.set('context.enable_lua_jit', enabled);
         });
*/
      radiant.call('radiant:get_config', 'collect_analytics')
         .done(function(o) {
            var enabled = o.collect_analytics != false;
            self.set('context.collect_analytics', enabled);
         });
   },

   _updateGfxTabPage: function(o) {
      var self = this;
      
      self.set('context.shadows_forbidden', !o.shadows.allowed);
      if (!o.shadows.allowed) {
         o.shadows.value = false;
      }
      self.set('context.shadows_enabled', o.shadows.value);
      self.set('context.shadow_quality', self.fromResToVal(o.shadow_quality.value, o.shadows.value))
      self.set('context.max_lights', o.max_lights.value)
      self.set('context.vsync_enabled', o.vsync.value);
      self.set('context.fullscreen_enabled', o.fullscreen.value);

      self.set('context.num_msaa_samples', o.msaa.value);
      self.set('context.draw_distance', o.draw_distance.value);

      self.set('context.enable_ssao', o.enable_ssao.value);

      self.set('context.high_quality_forbidden', !o.use_high_quality.allowed);
      if (!o.use_high_quality.allowed) {
         o.use_high_quality.value = false;
      }
      self.set('context.use_high_quality', o.use_high_quality.value);

      $('#aaNumSlider').slider({
         value: self.get('context.num_msaa_samples'),
         min: 0,
         max: 1,
         step: 1,
         disabled: self.get('low_quality'),
         slide: function( event, ui ) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' });
            $('#aaNumDescription').html(i18n.t('stonehearth:settings_aa_slider_' + ui.value));
         }
      }); 
      $('#aaNumDescription').html(i18n.t('stonehearth:settings_aa_slider_' + self.get('context.num_msaa_samples')));

      $('#shadowResSlider').slider({
         value: self.get('context.shadow_quality'),
         min: 0,
         max: 5,
         step: 1,
         disabled: self.get('context.shadows_forbidden'),
         slide: function( event, ui ) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' });
            $('#shadowResDescription').html(i18n.t('stonehearth:settings_shadow_' + ui.value));
         }
      });
      $('#shadowResDescription').html(i18n.t('stonehearth:settings_shadow_' + self.get('context.shadow_quality')));

      $('#maxLightsSlider').slider({
         value: self.get('context.max_lights'),
         min: 1,
         max: 250,
         step: 1,
         slide: function( event, ui ) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' });
            $('#maxLightsDescription').html(ui.value);
         }
      });
      $('#maxLightsDescription').html(self.get('context.max_lights'));

      $('#drawDistSlider').slider({
         value: self.get('context.draw_distance'),
         min: 500,
         max: 1000,
         step: 10,
         slide: function( event, ui ) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' });
            $('#drawDistDescription').html(ui.value);
         }
      });
      $('#drawDistDescription').html(self.get('context.draw_distance'));
   },

   getGraphicsConfig: function(persistConfig) {
      var newConfig = {
         "shadows" : $( "#shadowResSlider" ).slider( "value" ) > 0,
         "vsync" : $('#opt_enableVsync').is(':checked'),
         "fullscreen" : $('#opt_enableFullscreen').is(':checked'),
         "msaa" : $( "#aaNumSlider" ).slider( "value" ),
         "shadow_quality" :  $( "#shadowResSlider" ).slider( "value" ),
         "max_lights" :  $( "#maxLightsSlider" ).slider( "value" ),
         "persistConfig" : persistConfig,
         "draw_distance" : $( "#drawDistSlider" ).slider( "value" ),
         "use_high_quality" : $('#opt_useHighQuality').is(':checked'),
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
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
      var newConfig = this.getGraphicsConfig(persistConfig);
      radiant.call('radiant:set_config_options', newConfig);

      var audioConfig = this.getAudioConfig();
      radiant.call('radiant:set_audio_config', audioConfig);

      radiant.call('radiant:set_config', 'force_32_bit', $('#opt_force32bit').is(':checked'));
      //radiant.call('radiant:set_config', 'enable_lua_jit', $('#opt_enableLuaJit').is(':checked'));
      radiant.call('radiant:set_config', 'collect_analytics', $('#opt_enableCollectAnalytics').is(':checked'));
   },

   dismiss: function() {
      radiant.call('radiant:set_config_options', this.oldConfig);
   },

   applySettings: function() {
      this.applyConfig(true);
      this.destroy();
   },

   cancel: function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
      this.dismiss();
      this.destroy();
   },

});