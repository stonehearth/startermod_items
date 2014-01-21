
App.StonehearthShellView = App.ContainerView.extend({
   init: function() {
      this._super();
      var self = this;

      var json = {
         views : [
            //"StonehearthTitleScreenView"
            "StonehearthEmbarkView"
         ]
      };

      var views = json.views || [];
      $.each(views, function(i, name) {
         console.log(name);
         var ctor = App[name]
         if (ctor) {
            self.addView(ctor);
         }
      });
   }
});
