
App.StonehearthGameUiView = Ember.ContainerView.extend({
   init: function() {
      this._super();
      var self = this;

      // xxx: fetch this from the game...
      var json = {
         views : [
            "StonehearthUnitFrameView",
            "StonehearthObjectBrowserView",
            "StonehearthCalendarView",
            "StonehearthMainActionbarView",
            "StonehearthCanvasTestView"
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

      this._traceCalendar();
   },

   destroy: function() {
      this._super();
      this._calendarTrace.destroy();
   },

   addView: function(type, options) {
      console.log("adding view " + type);

      var childView = this.createChildView(type, {
         classNames: ['stonehearth-view']
      });
      childView.setProperties(options);

      if (childView.get('modal')) {
         var modalOverlay = App.gameView.addView('StonehearthModalOverlay', { modalView: childView });
         childView.modalOverlay = modalOverlay;
         this.pushObject(modalOverlay);
      }

      this.pushObject(childView);
      return childView;
   },

   getDate: function() {
      return this._date;
   },

   _traceCalendar: function() {
      var self = this;
      this._calendarTrace = radiant.trace('stonehearth_calendar.clock')
         .progress(function(json) {
            self._date = json.date;
         })
         .fail(function(e) {
            console.log(e);
         });
   },
});
