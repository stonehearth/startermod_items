App.StonehearthCalendarView = App.View.extend({
   templateName: 'stonehearthCalendar',

   init: function() {
      this._super();
      var self = this;
      
      self.set('context', {});

      radiant.call_obj('stonehearth', 'get_clock_object')
         .done(function(o) {
            this.trace = radiant.trace(o.clock_object)
               .progress(function(date) {
                  self.set('context.date', date);
               })
         });
   },

   destroy: function() {
      this._super();
      this.trace.destroy();
   },

   didInsertElement: function() {
      this._clock = $('#clock');
      this._sun = $('#sun');
      this._sunBody = $('#sunBody');
      this._sunRays = $('#sunRays');
      this._moon = $('#moon');
      this._daySky = $('#skyDay');
   },

   _updateClock: function() {
      if (this._clock == undefined) {
         return;
      }
      var date = this.get('context.date');

      if (!date) {
         return;
      }

      this.set('context.time', date.time);
      var seconds = date.second + (date.minute * 60) + (date.hour * 3600);

      var SECONDS_IN_DAY = 86400;
      var SECONDS_IN_HOUR = 3600;
      var CLOCK_WIDTH = this._clock.width();
      var CLOCK_HEIGHT = this._clock.height();
      var CLOCK_LEFT = this._clock.position().left;
      var CLOCK_TOP = this._clock.position().top;
      var SUN_WIDTH = this._sun.width();
      var SUN_HEIGHT = this._sun.width();

      var PATH_X_MIN = CLOCK_LEFT - SUN_WIDTH + 3;
      var PATH_X_MAX = CLOCK_LEFT + CLOCK_WIDTH - 3;
      var PATH_Y_MIN = CLOCK_TOP - SUN_HEIGHT + 8;
      var PATH_Y_MAX = CLOCK_TOP + CLOCK_HEIGHT - 8;

      var PATH_WIDTH = PATH_X_MAX - PATH_X_MIN;
      var PATH_HEIGHT = PATH_Y_MAX - PATH_Y_MIN;

      var PATH_SEGMENT_TIME = SECONDS_IN_DAY / 4;

      var DAWN = SECONDS_IN_HOUR * 6;
      var DAY_BEGINNING = SECONDS_IN_HOUR * 9;
      var DAY_END = SECONDS_IN_HOUR * 18;
      var DUSK = SECONDS_IN_HOUR * 21;

      // fade the sky
      var opacity = 0
      var skySeconds = seconds % SECONDS_IN_DAY;

      if (skySeconds > DAWN && skySeconds < DAY_BEGINNING ) {
         // morning
         opacity = (skySeconds - DAWN) / (DAY_BEGINNING - DAWN);
         //this._sunBody.css('background', 'url(/stonehearth_calendar/html/images/clock/sun.png)');
      } else if (skySeconds >= DAY_BEGINNING && skySeconds <= DAY_END) {
         // day
         opacity = 1;
         //this._sunBody.css('background', 'url(/stonehearth_calendar/html/images/clock/sun.png)');
      } else if (skySeconds >= DAY_END && skySeconds < DUSK ) {
         // evening
         opacity = 1 - ((skySeconds - DAY_END) / (DUSK - DAY_END));
         //this._sunBody.css('background', 'url(/stonehearth_calendar/html/images/clock/sun_asleep.png)');            
      } else {
         //night
         opacity = 0;
         //this._sunBody.css('background', 'url(/stonehearth_calendar/html/images/clock/sun_asleep.png)');            
      }

      this._daySky.css('opacity', opacity);
      this._sunRays.css('opacity', opacity);

      // the sun and moon's path around the sky
      var offsetSeconds = seconds + (SECONDS_IN_DAY * 5 / 8);
      var daySeconds = offsetSeconds % SECONDS_IN_DAY;
      var t = daySeconds % PATH_SEGMENT_TIME;

      var dx = Math.floor(PATH_WIDTH * (t / PATH_SEGMENT_TIME));
      var dy = Math.floor(PATH_HEIGHT * (t / PATH_SEGMENT_TIME));

      if (daySeconds < PATH_SEGMENT_TIME) {
         // sun on bottom, moon on top
         this._sun.css({'top': PATH_Y_MAX, 'left': PATH_X_MIN + dx}).css('z-index', 10);
         this._moon.css({'top': PATH_Y_MIN, 'left': PATH_X_MAX - dx}).css('z-index', 1);
      } else if (daySeconds < PATH_SEGMENT_TIME * 2) {            
         // sun on right, moon on left
         this._sun.css({'top': PATH_Y_MAX - dy, 'left': PATH_X_MAX}).css('z-index', 10);
         this._moon.css({'top': PATH_Y_MIN + dy, 'left': PATH_X_MIN});
      } else if (daySeconds < PATH_SEGMENT_TIME * 3) {
         // sun on top, moon on bottom
         this._sun.css({'top': PATH_Y_MIN, 'left': PATH_X_MAX - dx}).css('z-index', 1);
         this._moon.css({'top': PATH_Y_MAX, 'left': PATH_X_MIN + dx}).css('z-index', 10);
      } else {
         // sun on left, moon on right
         this._sun.css({'top': PATH_Y_MIN + dy, 'left': PATH_X_MIN}).css('z-index', 10);
         this._moon.css({'top': PATH_Y_MAX - dy, 'left': PATH_X_MAX});
      }

   }.observes('context.date')

});

$(document).ready(function(){

});
