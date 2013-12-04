App.StonehearthCalendarView = App.View.extend({
   templateName: 'stonehearthCalendar',

   init: function() {
      this._super();
      var self = this;
      
      self.set('context', {});

      $.get('/stonehearth/services/calendar/calendar_constants.json')
         .done(function(json) {
            self._constants = json;

            radiant.call('stonehearth:get_clock_object')
               .done(function(o) {
                  this.trace = radiant.trace(o.clock_object)
                     .progress(function(date) {
                        self.set('context.date', date);
                     })
               });
         })
   },

   destroy: function() {
      this._super();
      this.trace.destroy();
   },

   didInsertElement: function() {
      this._sun = $('#clock > #sun');
      this._sunBody = this._sun.find('#body');
      this._sunRays = this._sun.find('.ray');

      this._moon = $('#clock > #moon');
      this._moonBody = this._moon.find('#body');
      this._moonRays = this._moon.find('.ray');
   },

   _updateClock: function() {
      var self = this;

      if (this._sun == undefined) {
         return;
      }

      var date = this.get('context.date');

      if (!date) {
         return;
      }

      var hoursRemaining;

      if (date.hour >= this._constants.event_times.sunrise && date.hour < this._constants.event_times.sunset) {
         hoursRemaining = this._constants.event_times.sunset - date.hour;

         if (this._hoursRemaining != hoursRemaining) {

            if(!this._sunBody.is(':visible')) {
               radiant.call('radiant:play_sound', 'stonehearth:sounds:rooster_call' );
            }

            //transition to day
            this._moonBody.hide();
            this._moonRays.hide();
            this._sunBody.show();

            /*
            if(this._moonBody.is(':visible')) {
               this._moon.animate({ 'top': 0, 'left' : 120 }, 500, function() {
                  self._moonRays.hide();
                  self._moonBody.hide();

                  self._sun.css({'top' : -120, 'left' : -0});
                  self._sunBody.show();
                  self._sun.animate({'top' : 0,'left' : 0}, 500);

               });
            }
            */

            this._sun.find('#ray' + hoursRemaining).fadeIn();
            this._hoursRemaining = hoursRemaining;

            //Play music as the game starts
            var args = {
               'track': 'stonehearth:ambient:summer_day',
               'channel': 'ambient', 
               'volume' : 40
            };
            radiant.call('radiant:play_music', args);
         }   
      } else {
         if (date.hour < this._constants.event_times.sunrise) {
            hoursRemaining = this._constants.event_times.sunrise - date.hour
         } else {
            hoursRemaining = this._constants.event_times.sunrise + (this._constants.hours_per_day - date.hour)
         }

         if (this._hoursRemaining != hoursRemaining) {

            if(!this._moonBody.is(':visible')) {
               radiant.call('radiant:play_sound', 'stonehearth:sounds:owl_call' );
            }

            //transition to night
            this._sunBody.hide();
            this._sunRays.hide();
            this._moonBody.show();
            /*
            if(this._sunBody.is(':visible')) {            
               this._sun.animate({ 'top' : 0, 'left' : 120 }, 500, function() {
                  self._sunRays.hide();
                  self._sunBody.hide();

                  self._moon.css({'top' : -120, 'left' : -0});
                  self._moonBody.show();
                  self._moon.animate({'top' : 0,'left' : 0}, 500);

               });
            }
            */

            //show the moon, over and over...doh
            this._moonBody.show();
            this._sunRays.hide();
            this._sunBody.hide();

            this._moon.find('#ray' + hoursRemaining).fadeIn();
            this._hoursRemaining = hoursRemaining;

            //Play music as the game starts
            var args = {
               'track': 'stonehearth:ambient:summer_night',
               'channel': 'ambient',
               'volume' : 20
            };
            radiant.call('radiant:play_music', args);  
         }
      }

      this.set('context.hoursRemaining', this._hoursRemaining)

      //console.log(hoursRemaining);
   }.observes('context.date')

});

$(document).ready(function(){

});
