App.StonehearthEventLogView = App.View.extend({
	templateName: 'eventLog',

   init: function() {
      this._super();

      var self = this;
      radiant.call('stonehearth:get_event_tracker')
         .done(function(response) {
            console.log(response.tracker);
            self.trace = radiant.trace(response.tracker)
               .progress(function(data) {
                  self.addEvent(data);
               })
               .fail(function(e) {
                  console.log(e);
               });

            //self.set('uri', response.tracker);
         });

   },

   addEvent: function(event) {
      

      if (event && event.text) {
         var el = $('<div></div>')
            .html(event.text)
            .addClass('event')
            .addClass(event.type)
            .appendTo(this._container);

         if (event.type == 'warning') {
            el.pulse()
         }

         el.animate({ color: "#fff" }, 500);
      }

      this._container.find('.event:not(:nth-last-child(-n+10))')
         .fadeOut(2000, function() {
            $(this).remove();
         })
         
      //this._container.find(':nth-child(-n+5)').addClass('wtf')

   },

   reverse: function(){
       return this.get('context.events').toArray().reverse();
   }.property('events.@each').cacheable(),

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      this._container = $('#events');
   },

});
