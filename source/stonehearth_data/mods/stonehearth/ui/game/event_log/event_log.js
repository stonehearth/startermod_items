App.StonehearthEventLogView = App.View.extend({
	templateName: 'eventLog',

   components: {
      "stonehearth:commands": {
         commands: []
      },
      "unit_info": {}
   },

   init: function() {
      this._super();

      var self = this;
      radiant.call('stonehearth:get_events')
         .done(function(o) {
            this.trace = radiant.trace(o.events)
               .progress(function(data) {
                  self.set('context.events', data);
               })
         });
   },

   reverse: function(){
       return this.get('context.events').toArray().reverse();
   }.property('events.@each').cacheable(),

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {

   },

});
