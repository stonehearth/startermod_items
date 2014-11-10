App.StonehearthTaskItemView = App.View.extend({
   templateName: "taskItem",
});

App.StonehearthTasksView = App.View.extend({
	templateName: 'tasks',
   closeOnEsc: true,

   _components: {
      "groups" : {
         "tasks" : {
            "*" : {}
         }
      }
   },

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Tasks');
      self.get('context.model');

      radiant.call('stonehearth:get_tasks')
               .done(function(response) {
                  console.log('stonehearth:get_tasks returned', response)
                  this._trace = new StonehearthDataTrace(response.tasks, self._components);
                  this._trace.progress(function(model) {
                        self.set('context.model', model);

                        // Boo!  It can't convince Ember to re-render the entire view whenever
                        // any of the sub-objects of the model change.  So set a timer! =()
                        self._timer = setInterval(function() {
                           if (self._timer) {
                              self._rebuildGroupsArray();
                           }
                        }, 200);
                     })
               });
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
         this._trace = null;
      }
      if (self._timer) {
         clearInterval(self._timer);
         self._timer = null;
      }
      this._super();
   },

   _rebuildGroupsArray: function() {
      var self = this;

      var objectToArray = function(obj) {
         var arr = [];
         $.each(obj, function(k, v) {
            if (k != "__self" && obj.hasOwnProperty(k)) {
               arr.push(v)
            };
         });
         return arr;
      };

      var groups = this.get('context.model.groups');

      var groupsArray = objectToArray(groups);      
      $.each(groupsArray, function(k, group) {
         var name = i18n.t(group.name);
         group.set('name', name);
         group.set('tasksArray', objectToArray(group.tasks));
      });

      this.set('context.model.groupsArray', groupsArray);
    }.observes('context.model.groups'),
});
