App.StonehearthRedAlertWidget = App.View.extend({
   templateName: 'stonehearthRedAlert',
   classNames: ['flex', 'fullScreen'],

   didInsertElement: function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:scenarios:redalert'} );
      this._super();

      this.$('#redAlert').click(function() {
         App.stonehearthClient.rallyWorkers();
      });

      this.$('#redAlert').pulse();

   }
});
