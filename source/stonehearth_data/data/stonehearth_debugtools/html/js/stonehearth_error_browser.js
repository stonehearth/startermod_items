App.StonehearthErrorBrowserView = App.View.extend({
   templateName: 'errorBrowser',
   components: {
      "entries": []
   },

   init: function() {
      this._super();
   },
   didInsertElement: function() {
      var self = this;
      console.log('inserted error browser');

      this.set('uri', '/o/named_objects/client/error_browser');

      var self = this;
      $("#errorBrowser").on("click", "a", function(event) {
         var error_uri = $(this).attr('error_uri');
         if (self.currentView) {
            self.currentView.destroy();
         }
         self.currentView = App.gameView.addView(App.StonehearthErrorDialogView, { uri : error_uri});
      });
   },
});
