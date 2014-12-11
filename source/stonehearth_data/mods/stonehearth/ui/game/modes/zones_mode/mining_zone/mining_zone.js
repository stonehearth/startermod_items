App.StonehearthMiningZoneView = App.View.extend({
   templateName: 'stonehearthMiningZone',
   uriProperty: 'model',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:mining_zone" : {}
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      self.$('#name').keypress(function(e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val());
            $(this).blur();
         }
      });

      self.$('button.ok').click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:submenu_select'} );
         self.destroy();
      });

      self.$('button.warn').click(function() {
         radiant.call('stonehearth:destroy_entity', self.uri)
         self.destroy();
      });

      self.$('#disableButton').click(function() {
         var mining_zone_component = self.get('model.stonehearth:mining_zone');
         var enabled = mining_zone_component.enabled;
         radiant.call_obj(mining_zone_component.__self, 'set_enabled_command', !enabled);
      })
   }
});
