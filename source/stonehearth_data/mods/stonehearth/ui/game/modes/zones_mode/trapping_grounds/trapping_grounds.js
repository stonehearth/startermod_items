App.StonehearthTrappingGroundsView = App.View.extend({
   templateName: 'stonehearthTrappingGrounds',
   uriProperty: 'model',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:trapping_grounds" : {}
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      new StonehearthInputHelper(this.$('#name'), function (value) {
            radiant.call('stonehearth:set_display_name', self.uri, value)
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
         //xxx toggle zone enabled and disabled state.
      })
   }
});
