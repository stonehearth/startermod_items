App.StonehearthTrappingGroundsView = App.View.extend({
   templateName: 'stonehearthTrappingGrounds',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:trapping_grounds" : {}
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      this.$('#name').keypress(function(e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val());
            $(this).blur();
         }
      });

      this.$('button.ok').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:submenu_select' );
         self.destroy();
      });

      this.$('button.warn').click(function() {
         radiant.call('stonehearth:destroy_entity', self.uri)
         self.destroy();
      });
   }
});
