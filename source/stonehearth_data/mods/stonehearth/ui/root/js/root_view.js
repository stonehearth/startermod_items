App.RootView = Ember.ContainerView.extend({

   init: function() {
      this._super();
      var self = this;

      // create the views
      this._debugView = this.createChildView(App["StonehearthDebugView"]);
      this._gameView = this.createChildView(App["StonehearthGameUiView"]);
      this._shellView = this.createChildView(App["StonehearthShellView"]);

      // push em
      this.pushObject(this._gameView)
      this.pushObject(this._shellView)
      this.pushObject(this._debugView)

      // accessors for easy access throughout the app
      App.gameView = this._gameView;
      App.shellView = this._shellView;
      App.debugView = this._debugView;

      this._game_mode_manager = new GameModeManager();

      App.gotoGame = function() {
         self.gotoGame();
      }

      App.gotoShell = function() {
         self.gotoShell();
      }

      App.getGameMode = function() {
         return self._game_mode_manager.getGameMode();
      }

      App.setGameMode = function(mode) {
         self._game_mode_manager.setGameMode(mode);
      }

      App.setVisionMode = function(mode) {
         self._game_mode_manager.setVisionMode(mode);
      }

      App.getVisionMode = function() {
         return self._game_mode_manager.getVisionMode();
      }
   },

   didInsertElement: function() {
      var state = App.options['state'];
      if (App.options['skip_title'] || state == 'load_finished') {
         App.gameView._addViews(App.gameView.views.complete);
         App.gotoGame();
      } else {
         App.gotoShell();
      }

      $(document).trigger('stonehearthReady');
   },

   gotoGame: function() {

      radiant.call('radiant:set_draw_world', {
            'draw_world': true
         });

      $('#' + this._shellView.elementId).hide();
      $('#' + this._gameView.elementId).show();

      radiant.call('radiant:play_music', {
            'track': {
                  'type' : 'one_of',
                  'items' : [
                     'stonehearth:music:levelmusic_spring_day_01',
                     'stonehearth:music:levelmusic_spring_day_02',
                     'stonehearth:music:levelmusic_spring_day_03',
                     ]
                  }, 
               'channel': 'bgm',
               'fade': 1400,
               'volume' : 35 
         });         
         
      radiant.call('radiant:play_music', {
            'track': 'stonehearth:ambient:summer_day',
            'channel': 'ambient', 
            'volume' : 60
         });  

      App.stonehearthTutorials = new StonehearthTutorialManager();

      /*
      setTimeout(function() {
         App.stonehearthTutorials.start();
      }, 1000);
      */
   },

   gotoShell: function() {
      $('#' + this._gameView.elementId).hide();
      $('#' + this._shellView.elementId).show();

      radiant.call('radiant:play_music', {
            'track': 'stonehearth:music:title_screen', 
            'channel' : 
            'bgm'} 
         );      
   }

});
