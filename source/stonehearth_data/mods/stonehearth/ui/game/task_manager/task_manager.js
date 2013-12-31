App.StonehearthTaskManagerView = App.View.extend({
   templateName: 'stonehearthTaskManager',

   init: function() {
      this._super();
      var self = this;
      self.set('context', {});

      radiant.call('radiant:game:start_task_manager')
            .progress(function (response) {
               var foo = JSON.stringify(response)
               self.set('context.data', foo);
            })
   },

   destroy: function() {
      this._super();
      this.trace.destroy();
   }

});
