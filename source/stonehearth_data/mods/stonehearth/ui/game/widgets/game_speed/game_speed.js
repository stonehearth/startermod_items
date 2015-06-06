App.StonehearthGameSpeedWidget = App.View.extend({
   templateName: 'stonehearthGameSpeed',

   speeds: {
      'PAUSED' : 0, 
      'PLAY'   : 1, 
      'FASTFORWARD' : 2      
   },
      
   //On startup, get the game_speed service, so we can listen in on speed changes
   init: function() {
      var self = this;
      this._super();

      radiant.call('stonehearth:get_game_speed_service')
         .done(function(response){
            var uri = response.game_speed_service;
            self.set('uri', uri);
         });
   },

  //When the current speed changes, toggle the buttons to match
   _set_button_by_speed: function() {
      var newSpeed = this.get('context.curr_speed');
      
      //find currently active button, and unselect it
      this._toggle_button_by_speed(this.currSpeed, false);
    
      //find next button, and select it
      this._toggle_button_by_speed(newSpeed, true);

      //Make the current speed the new speed. 
      this.currSpeed = newSpeed;
      
   }.observes('context.curr_speed'), 

   //Given a speed, and a selected state, make the 
   //correct button match that state
   _toggle_button_by_speed: function(speed, selected) {
      switch (speed) {
         case this.speeds.PAUSED:
            this.set('context.isPaused', selected);
            break;
         case this.speeds.PLAY:
            this.set('context.isPlay', selected);
            break;
         case this.speeds.FASTFORWARD:
            this.set('context.isFF', selected);
            break;
         default:
            break;
      }
   },

   //On first load, get the speed that PLAY should be
   _setInitialSpeedState: function(){
      var self = this;

      //Until we're told otherwise, we assume that we're running at PLAY speeds
      this.currSpeed = this.speeds.PLAY;
      this.set('context.isPaused', false);
      this.set('context.isPlay', true);
      this.set('context.isFF', false);

      radiant.call('stonehearth:get_default_speed')
         .done(function(e){
            self.speeds.PLAY = e.default_speed;
         });
   },

   _showPausedIndicator: function() {
      var paused = this.get('context.isPaused');
      return;

      if (paused) {
         this.$('#pausedIndicator').show();
      } else {
         this.$('#pausedIndicator').hide();
      }
   }.observes('context.isPaused'),

   didInsertElement: function() {
      var self = this;

      if (!this.initialized) { 
         this._setInitialSpeedState();
         this.initialized = true;
      }

      //On click, change the speed. 
      this.$('#pauseButton').click(function() {
            radiant.call('stonehearth:set_player_game_speed', self.speeds.PAUSED);
      });

      this.$('#playButton').click(function() {
            radiant.call('stonehearth:set_player_game_speed', self.speeds.PLAY);
      });

      this.$('#ffButton').click(function() {
            radiant.call('stonehearth:set_player_game_speed', self.speeds.FASTFORWARD);
      });

      this.$('#pausedIndicator').on( 'click', '#inner', function() {
         self.$('#playButton').click();
      });

      self._addHotkeys();

      // tooltip
      var buttons = ['pause', 'play', 'ff'];
      radiant.each(buttons, function(i, buttonName) {
         var button = this.$('#'+ buttonName + 'Button');
         var description_key = 'stonehearth:' + buttons[i] + '_description';
         button.tooltipster({
         position: 'bottom',
         content: $('<div class=title>' + i18n.t('stonehearth:' + buttonName + '_title') + '</div>' + 
                    '<div class=description>' + i18n.t(description_key) + '</div>' + 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + button.attr('hotkey')  + '</span></div>')
         });
      });

   }
});
