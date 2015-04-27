$(document).ready(function(){
      // hide the gui on alt+z
      $(document).bind('keydown', 'alt+z', function() {
         App.gameView.$().toggle();
         App.debugView.$().toggle();
      });
});

App.StonehearthGameUiView = App.ContainerView.extend({
   init: function() {
      this._super();
      this.views = {
         initial: [
            "StonehearthEventLogView",
            "StonehearthCalendarView",
            "StonehearthVersionView",
            ],
         complete: [
            'StonehearthStartMenuView',
            'StonehearthTaskManagerView',
            'StonehearthGameSpeedWidget',
            'StonehearthBuildingVisionWidget',
            'StonehearthTerrainVisionWidget',
            'StonehearthHudScoreWidget',
            'StonehearthUnitFrameView',
         ]
      };
      
      this._addViews(this.views.initial);

      // xxx, move the calendar to a data service like population
      this._traceCalendar();
   },

   destroy: function() {
      this._super();
      this._calendarTrace.destroy();
   },

   getDate: function() {
      return this._date;
   },

   _addViews: function(views) {
      var views = views || [];
      var self = this;
      $.each(views, function(i, name) {
         var ctor = App[name]
         if (ctor) {
            self.addView(ctor);
         }
      });
   },

   _traceCalendar: function() {
      var self = this;
      radiant.call('stonehearth:get_clock_object')
         .done(function(o) {
            this.trace = radiant.trace(o.clock_object)
               .progress(function(date) {
                  self._date = date;
               })
               .fail(function(e) {
                  console.error('could not trace clock_object', e)
               });
         })
         .fail(function(e) {
            console.error('could not get clock_object', e)
         });
   },
});
