
App.StonehearthGameUiView = App.ContainerView.extend({
   init: function() {
      this._super();
      this.views = {
         initial: [
            "StonehearthHelpCameraView",
            "StonehearthEventLogView",
            "StonehearthCalendarView"
            ],
         complete: [
            "StonehearthStartMenuView",
            "StonehearthTaskManagerView",
            "StonehearthGameSpeedWidget",
            "StonehearthBuildingVisionWidget"
         ]
      };
      
      this._addViews(this.views.initial);
   },

   destroy: function() {
      this._super();
      this._calendarTrace.destroy();
   },

   getDate: function() {
      return this._date;
   },

   initGameServices: function() {
      if (!this._gameServicesInitialized) {
         App.population = new StonehearthPopulation();
         App.inventory = new StonehearthInventory();
         App.bulletinBoard = new StonehearthBulletinBoard();
         
         this._traceCalendar();

         this._gameServicesInitialized = true;         
      }
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
