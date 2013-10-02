App.StonehearthView = Ember.ContainerView.extend({

   _gameView: null,
   _titleScreenView: null,

   init: function() {
      this._super();
      var self = this;

      // create the views
      this._gameView = this.createChildView(App["StonehearthGameUiView"]);
      this._titleScreenView = this.createChildView(App["StonehearthTitleScreenView"]);

      // push em
      this.pushObject(this._gameView)
      this.pushObject(this._titleScreenView)

      // accessors for easy access throughout the app
      App.gameView = this._gameView;
      App.titleScreenView = this._titleScreenView;

      App.gotoGame = function() {
         self.gotoGame();
      }

      App.gotoTitleScreen = function() {
         self.gotoTitleScreen();
      }
   },

   didInsertElement: function() {
      if (App.options['skip_title']) {
         App.gotoGame();
      }
   },

   gotoGame: function() {
      $('#' + this._titleScreenView.elementId).hide();
   },

   gotoTitleScreen: function() {
      $('#' + this._titleScreenView.elementId).show();
   }
});
