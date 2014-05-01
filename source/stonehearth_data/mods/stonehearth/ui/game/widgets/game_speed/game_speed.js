App.StonehearthGameSpeedWidget = App.View.extend({
   templateName: 'stonehearthGameSpeed',

   isPlaying: true, 

   didInsertElement: function() {
      var self = this;
      this.$('#playPauseButton').click(function() {
         console.log('toggle play/pause!');
         $(this).toggleClass( "showPlay" );
         $(this).toggleClass( "showPause" );

         self.isPlaying = !self.isPlaying;

         if (self.isPlaying) {
            App.stonehearthClient.setPaused(false)
            radiant.call('radiant:game:set_game_speed', App.stonehearthClient.defaultGameSpeed());
         } else {
            //pause the game
            App.stonehearthClient.setPaused(true)
            radiant.call('radiant:game:set_game_speed', 0);
         }
      });

      //TODO, add a tooltip
   }
});
