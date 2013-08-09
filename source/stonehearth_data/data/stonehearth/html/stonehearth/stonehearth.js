
App.StonehearthView = Ember.ContainerView.extend({
   init: function() {
      this._super();
      var self = this;
      
      // xxx: fetch this from the game...
      var json = {
         views : [
            "StonehearthUnitFrameView",
            "StonehearthObjectBrowserView",
            "StonehearthCalendarView",
            "StonehearthMainActionbarView"
         ]
      };

      var views = json.views || [];
      $.each(views, function(i, name) {
         console.log(name);
         var ctor = App[name]
         if (ctor) {
            console.log("adding view " + App[name]);
            self.addView(ctor);
         }
      });
   },

   addView: function(type, options) {
      var childView = this.createChildView(type, {
         classNames: ['stonehearth-view']
      });
      childView.setProperties(options);
      this.pushObject(childView)
      return childView;
   }

});
