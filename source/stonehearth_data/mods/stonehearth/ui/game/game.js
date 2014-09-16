$(document).ready(function(){
      $(top).bind('keyup', function(e){
         if (e.keyCode == 9)  { // tab
            //xxx, bring this back when we can distinguish between tab and alt+tab
            //App.gameView.$().toggle();
            //App.debugView.$().toggle();
         }
      });
});

App.StonehearthGameUiView = App.ContainerView.extend({
   init: function() {
      this._super();
      this.views = {
         initial: [
            'StonehearthHelpCameraView',
            'StonehearthEventLogView',
            'StonehearthCalendarView'
            ],
         complete: [
            'StonehearthStartMenuView',
            'StonehearthTaskManagerView',
            'StonehearthGameSpeedWidget',
            'StonehearthBuildingVisionWidget',
            'StonehearthBulletinListWidget',
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
         console.log(name);
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
