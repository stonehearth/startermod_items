App.StonehearthGameSpeedWidget = App.View.extend({
   templateName: 'stonehearthGameSpeed',
   uriProperty: 'model',

   speeds: {
      'PAUSED' : 0, 
      'PLAY'   : 1, 
      'FASTFORWARD' : 2,
      'SPEEDTHREE' : 9
   },
      
   //On startup, get the game_speed service, so we can listen in on speed changes
   init: function() {
      var self = this;
      this._super();

      radiant.call('radiant:get_config', 'mods.stonehearth.enable_speed_three')
         .done(function(response) {
            self.set('context.enableSpeedThree', response['mods.stonehearth.enable_speed_three']);
         })

      radiant.call('stonehearth:get_game_speed_service')
         .done(function(response){
            var uri = response.game_speed_service;
            self.set('uri', uri);
         });
   },

  //When the current speed changes, toggle the buttons to match
   _set_button_by_speed: function() {
      var newSpeed = this.get('model.curr_speed');
      
      //find currently active button, and unselect it
      this._toggle_button_by_speed(this.currSpeed, false);
    
      //find next button, and select it
      this._toggle_button_by_speed(newSpeed, true);

      //Make the current speed the new speed. 
      this.currSpeed = newSpeed;
      
   }.observes('model.curr_speed'), 

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
         case this.speeds.SPEEDTHREE:
            this.set('context.isSpeedThree', selected);
            break;
         default:
            break;
      }
   },

   //On first load, get the speed that PLAY should be
   _setInitialSpeedState: function(){
      var self = this;

      //Until we're told otherwise, we assume that we're running at PLAY speeds
      self.currSpeed = self.speeds.PLAY;
      self.set('context.isPaused', false);
      self.set('context.isPlay', true);
      self.set('context.isFF', false);
      self.set('context.isSpeedThree', false);

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

   _updateSpeedThreeButton: function() {
      var self = this;

      if (!self.get('context.enableSpeedThree')) {
         self.$('#speedThreeButton').hide();
         return;
      }

      if (self.initialized) {
         self.$('#speedThreeButton').click(function() {
            radiant.call('stonehearth:set_player_game_speed', self.speeds.SPEEDTHREE);
         });
         self.$('#speedThreeButton').show();
         self._createButtonTooltip('speedThree');
         self._addHotkeys();
      }
   }.observes('context.enableSpeedThree'),

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

      // tooltip
      var buttons = ['pause', 'play', 'ff', 'speedThree'];
      radiant.each(buttons, function(i, buttonName) {
         self._createButtonTooltip(buttonName);
      });

      self._updateSpeedThreeButton();
      self._addHotkeys();
   },
   _createButtonTooltip: function(buttonName) {
      var button = this.$('#'+ buttonName + 'Button');
      var description_key = 'stonehearth:' + buttonName + '_description';
      button.tooltipster({
      position: 'bottom',
      content: $('<div class=title>' + i18n.t('stonehearth:' + buttonName + '_title') + '</div>' + 
                 '<div class=description>' + i18n.t(description_key) + '</div>' + 
                 '<div class=hotkey>' + i18n.t('hotkey') + ' <span class=key>' + button.attr('hotkeyName')  + '</span></div>')
      });
   }

});
