App.StonehearthGameSpeedWidget = App.View.extend({
   templateName: 'stonehearthGameSpeed',

   //Consider changing the name of this variable if we have 3 different speed states + a pause button.
   //The current implementation reflects the current design of the UI.
   isPlaying: true, 
   curr_speed: 1,
   default_speed: 1,

   _togglePlayPause: function() {
      console.log('toggle play/pause!');
      $('#playPauseButton').toggleClass( "showPlay" );
      $('#playPauseButton').toggleClass( "showPause" );
   },

   //On first load, get the player's speed
   _setInitialSpeedState: function(){
      var self = this;
      radiant.call('stonehearth:get_player_speed')
         .done(function(e){
            self.curr_speed = e.player_speed;
            self.default_speed = e.default_speed
            if (self.curr_speed > 0) {
               self.isPlaying = true;
            } else {
               self.isPlaying = false;
               //Whoops, the UI is in the wrong default state
               self._togglePlayPause();
            }
         });
   },

   //TODO: when we implement fast forward, we'll pass a value
   //into the call based on which button is pressed.
   //Right now, there is only 1 play speed, so we just save it as "default_speed"
   didInsertElement: function() {
      var self = this;

      this._setInitialSpeedState();

      this.$('#playPauseButton').click(function() {
         self._togglePlayPause();

         self.isPlaying = !self.isPlaying;

         if (self.isPlaying) {
            radiant.call('stonehearth:set_player_game_speed', self.default_speed);
         } else {
            radiant.call('stonehearth:set_player_game_speed', 0);
         }
      });

      /*$(obj).tooltipster({
   88              position: 'right'
   89           });
*/

      // tooltip
      this.$('#playPauseButton').tooltipster({
         position: 'left',
         content: $('<div class=title>' + i18n.t('stonehearth:play_pause_title') + '</div>' + 
                    '<div class=description>' + i18n.t('stonehearth:play_pause_description') + '</div>' /*+ 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + $(this).attr('hotkey')  + '</span></div>'*/)
      });

   }
});
