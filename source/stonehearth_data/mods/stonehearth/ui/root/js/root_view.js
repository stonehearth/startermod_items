App.RootController = Ember.Controller.extend({
   needs: ['save'],

   _autoSave: function() {
      var self = this;
      var saveController = self.get('controllers.save');
      var enabled = saveController.get('auto_save');
      
      if (enabled) {
         saveController.send('saveGame', 'auto_save');
      }      
   },

   actions: {
      // every 5 minutes, check if autosave is enabled, and if it is, save.
      tryAutoSave: function(start) {
         var self = this;

         if (start) {
            this._intervalTicket = setInterval(function() { 
                  self._autoSave();
               }, 5 * 60 * 1000);
         } else {
            clearInterval(this._intervalTicket);
         }
      },
   },
}),

App.RootView = Ember.ContainerView.extend({

   init: function() {
      this._super();
      var self = this;

      // create the views
      this._debugView = this.createChildView(App["StonehearthDebugView"]);
      this._gameView  = this.createChildView(App["StonehearthGameUiView"]);
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
      };

      App.gotoShell = function() {
         self.gotoShell();
      };

      App.getGameMode = function() {
         return self._game_mode_manager.getGameMode();
      };

      App.setGameMode = function(mode) {
         self._game_mode_manager.setGameMode(mode);
      };

      App.setVisionMode = function(mode) {
         self._game_mode_manager.setVisionMode(mode);
      };

      App.getVisionMode = function() {
         return self._game_mode_manager.getVisionMode();
      };

      App.waitForFrames = function(numFrames, callback) {
         var f = function(numFramesLeft, callback) {
            if (numFramesLeft <= 0) {
               callback();
            } else {
               App.waitForFrames(numFrames - 1, callback);
            }
         };

         window.requestAnimationFrame(function(frameTime) {
            f(numFrames, callback);
         });
      };

      App.tooltipHelper = new StonehearthTooltipHelper();
   },

   didInsertElement: function() {
      var currentScreen = App.options['current_screen'];
      if (currentScreen == "title_screen") {
         App.gotoShell();
      } else {                  
         App.gameView._addViews(App.gameView.views.complete);
         App.gotoGame();
      }
      $(document).trigger('stonehearthReady');
   },

   gotoGame: function() {
      radiant.call('radiant:show_game_screen');

      $('#' + this._shellView.elementId).hide();
      $('#' + this._gameView.elementId).show();

      App.stonehearthTutorials = new StonehearthTutorialManager();

      $(document).trigger('stonehearthGameStarted');

      this.get('controller').send('tryAutoSave', true);
      
      setTimeout(function() {
         App.stonehearthTutorials.start();
      }, 1000);
      
   },

   gotoShell: function() {
      $('#' + this._gameView.elementId).hide();
      $('#' + this._shellView.elementId).show();

      this.get('controller').send('tryAutoSave', false);
   }

});
