
App.StonehearthDebugView = App.ContainerView.extend({
   init: function() {
      this._super();
      var self = this;

      var json = {
         views : [
            "StonehearthErrorBrowserView",
            "StonehearthConsoleView"
         ]
      };

      var views = json.views || [];
      $.each(views, function(i, name) {
         var ctor = App[name]
         if (ctor) {
            self.addView(ctor);
         }
      });

      // StonehearthDebugDockView is defined in stonehearth_debugtools, which
      // may not be installed.  Shouldn't it create the dock?  Yes, no?  -- tony
      if (App.StonehearthDebugDockView) {
         this._dock = this.addView(App.StonehearthDebugDockView);
      }
   }

});
