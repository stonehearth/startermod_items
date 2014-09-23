App.StonehearthAnalyticsOptView = App.View.extend({
        templateName: 'analyticsOpt',

   init: function() {
      this._super();
   },

   actions: {
      optIn: function(optIn) {
         this._doOpt(optIn);  
         $("#buttons").animate({ opacity: 0 }, 300, function() {
            if (optIn) {
               $("#thanks").fadeIn();
               setTimeout(function() {
                  $('#analyticsOptView').fadeOut(800);
               }, 2000)
            } else {
               $('#analyticsOptView').fadeOut(800);
            }
         });
      }
   },

   _doOpt: function(optIn) {
      radiant.call('radiant:set_collection_status', optIn)
   }
});