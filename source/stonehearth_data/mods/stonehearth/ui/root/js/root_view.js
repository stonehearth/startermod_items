App.RootView = Ember.ContainerView.extend({

   init: function() {
      this._super();
      var self = this;

      // create the views
      this._gameView = this.createChildView(App["StonehearthGameUiView"]);
      this._shellView = this.createChildView(App["StonehearthShellView"]);

      // push em
      this.pushObject(this._gameView)
      this.pushObject(this._shellView)

      // accessors for easy access throughout the app
      App.gameView = this._gameView;
      App.shellView = this._shellView;

      App.gotoGame = function() {
         self.gotoGame();
      }

      App.gotoShell = function() {
         self.gotoShell();
      }
   },

   didInsertElement: function() {
      if (App.options['skip_title']) {
         App.gameView._addViews(App.gameView.views.complete);
         App.gotoGame();
      } else {
         App.gotoShell();
      }
   },

   gotoGame: function() {
      $('#' + this._shellView.elementId).hide();
      $('#' + this._gameView.elementId).show();


      radiant.call('radiant:play_music', {
            'track': 'stonehearth:music:world_start',
            'channel' : 'bgm',
            'fade': 500
         });         
         
      radiant.call('radiant:play_music', {
            'track': 'stonehearth:ambient:summer_day',
            'channel': 'ambient', 
            'volume' : 40
         });  
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
