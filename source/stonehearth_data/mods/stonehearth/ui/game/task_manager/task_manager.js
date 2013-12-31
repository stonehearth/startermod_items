App.StonehearthTaskManagerView = App.View.extend({
   templateName: 'stonehearthTaskManager',

   init: function() {
      this._super();
      var self = this;
      self.set('context', {});

      radiant.call('stonehearth:get_clock_object')
         .done(function(o) {
            this.trace = radiant.trace(o.clock_object)
               .progress(function(response) {
                  self.set('context.data', response);
               })
         });
   },

   destroy: function() {
      this._super();
      this.trace.destroy();
   }

});
