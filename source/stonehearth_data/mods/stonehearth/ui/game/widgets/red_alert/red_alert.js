App.StonehearthRedAlertWidget = App.View.extend({
   templateName: 'stonehearthRedAlert',

   didInsertElement: function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:scenarios:redalert' );
      this._super();

      this.$('#redAlert').click(function() {
         App.stonehearthClient.redAlert();
      });

      this.$('#redAlert').pulse();

   }
});
