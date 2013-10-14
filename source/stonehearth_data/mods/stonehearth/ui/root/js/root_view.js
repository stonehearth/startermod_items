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

      App.gotoShell();
   },

   didInsertElement: function() {
      if (App.options['skip_title']) {
         App.gotoGame();
         App.gameView._addViews(App.gameView.views.complete);
      }
   },

   gotoGame: function() {
      $('#' + this._shellView.elementId).hide();
      $('#' + this._gameView.elementId).show();
   },

   gotoShell: function() {
      $('#' + this._gameView.elementId).hide();
      $('#' + this._shellView.elementId).show();
   }
});
