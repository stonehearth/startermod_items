App.StonehearthAnalyticsOptView = App.View.extend({
        templateName: 'analyticsOpt',

   init: function() {
      this._super();
      var self = this;

      radiant.call('radiant:get_collection_status')
         .done(function(o) {
            if (o.collection_status) {
               radiant.call('radiant:send_performance_stats');
            }
         });  
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
      if (optIn) {
         radiant.call('radiant:send_design_event', 'probe9:opt_in');
         radiant.call('radiant:send_performance_stats');
      }
   }
});