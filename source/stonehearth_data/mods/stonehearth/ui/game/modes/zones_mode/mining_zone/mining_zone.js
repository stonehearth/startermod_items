App.StonehearthMiningZoneView = App.View.extend({
   templateName: 'stonehearthMiningZone',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:mining_zone" : {}
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      radiant.call('stonehearth:get_mining_zone_enabled', self.uri)
         .done(function(response) {
            self._mining_zone_enabled = response.enabled;
            // Causes button flicker if the returned value is not the default
            // What's the right pattern for this? Should we pass this in as a parameter to the view instead?
            self._setDisabledButtonText(self._mining_zone_enabled);
         });

      this.$('#name').keypress(function(e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val());
            $(this).blur();
         }
      });

      this.$('button.ok').click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:submenu_select'} );
         self.destroy();
      });

      this.$('button.warn').click(function() {
         radiant.call('stonehearth:destroy_entity', self.uri)
         self.destroy();
      });

      this.$('#disableButton').click(function() {
         var enabled = !self._mining_zone_enabled;

         radiant.call('stonehearth:set_mining_zone_enabled', self.uri, enabled)
            .done(function() {
               self._mining_zone_enabled = enabled;
               self._setDisabledButtonText(self._mining_zone_enabled);
            });

         //$('#disableButton').html(i18n.t('stonehearth:mining_zone_enable'));
      })
   },

   _setDisabledButtonText: function(enabled) {
      var text;

      if (enabled) {
         text = i18n.t('stonehearth:mining_zone_disable');
      } else {
         text = i18n.t('stonehearth:mining_zone_enable');
      }
      self.$('#disableButton').html(text);
   }
});
