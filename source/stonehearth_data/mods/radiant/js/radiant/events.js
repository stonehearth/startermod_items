console.log('loading events...');

(function () {
   var RadiantEvents = SimpleClass.extend({

      init: function() { 
      },

      poll: function () {
         return;

         var self = this;
         radiant.call('radiant:get_events')
            .done(function (data) {
               $.each(data, function (_, o) {
                  $(top).trigger('radiant:events:*', o);
                  $(top).trigger(o.type, o.data);
               });
            })
            .error(function(e) {
               radiant.report_error('error returned from radiant:get_events', e)
            })
            .fail(function (jqxhr, statusTxt, errorThrown) {
               console.log("poll error: (status:" + statusTxt + " error:" + errorThrown + ")");
            })
            .always(function () {
               self.poll();
            });
      },
   });
   radiant.events = new RadiantEvents();
})();
