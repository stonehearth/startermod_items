App.StonehearthTaskItemView = App.View.extend({
   templateName: "taskItem",
});

App.StonehearthTasksView = App.View.extend({
	templateName: 'tasks',
   closeOnEsc: true,

   _components: {
      "groups" : {
         "tasks" : {
            "*" : {
               "source" : {
                  "unit_info" : {},
                  "stonehearth:stockpile" : {}
               }
            }
         }
      }
   },

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Tasks');
      self.get('context.model');

      radiant.call('radiant:get_config', 'stonehearth_tasks_view')
         .done(function(response) {
            self._config = response.stonehearth_tasks_view || {};
            self._applyConfig();
         });

      radiant.call('stonehearth:get_tasks')
               .done(function(response) {
                  console.log('stonehearth:get_tasks returned', response)
                  this._trace = new StonehearthDataTrace(response.tasks, self._components);
                  this._trace.progress(function(model) {
                        self.set('context.model', model);

                        // Boo!  It can't convince Ember to re-render the entire view whenever
                        // any of the sub-objects of the model change.  So set a timer! =()
                        /*
                        self._timer = setInterval(function() {
                           if (self._timer) {
                              self._rebuildGroupsArray();
                           }
                        }, 200);
                        */
                     })
               });
   },

   didInsertElement: function() {
      var self = this;  
      this._super();

      // tab buttons and pages
      this.$().on('click', '.tabButton', function() {
         var tool = $(this);
         var tabId = tool.attr('tab');
         var tab = self.$('#' + tabId);

         if (!tab) {
            return;
         }

         // show the correct tab page
         self.$('.tabPage').hide();
         tab.show();
         if (self._config) {
            self._config.selected_tab_id = tabId;
            radiant.call('radiant:set_config', 'stonehearth_tasks_view', self._config);
         }

         // activate the tool
         self.$('.tabButton').removeClass('active');
         tool.addClass('active');
      });

      // show toolbar control
      this.$().on('click', '.row', function() {        
         var selected = $(this).hasClass('selected'); // so we can toggle!
         self.$('.row').removeClass('selected');
         
         if (!selected) {
            $(this).addClass('selected');
         }
      });
   },

   actions: {
      removeStockpile: function(stockpile_uri) {
         radiant.call('stonehearth:destroy_entity', stockpile_uri)
      },
      pauseStockpile: function(stockpile_uri, paused) {
         var active = !paused;
         radiant.call_obj(stockpile_uri, 'set_active_command', active)
      },
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

      var objectToArray = function(obj, tweak_obj_fn) {
         var arr = [];
         $.each(obj, function(k, v) {
            if (k != "__self" && obj.hasOwnProperty(k)) {
               if (tweak_obj_fn(v)) {
                  arr.push(v)
               }
            };
         });
         return arr;
      };

      var groups = this.get('context.model.groups');

      var groupsArray = objectToArray(groups, function(group) {
         if (!group.published) {
            return false;
         }

         var displayName = i18n.t(group.name);
         group.set('tabButtonIcon', '/stonehearth/ui/game/tasks/images/buttons/' + group.name + '.png');
         group.set('taskIcon', '/stonehearth/ui/game/tasks/images/tasks/' + group.name + '.png');
         group.set('displayName', displayName);

         var tasksArray = objectToArray(group.tasks, function(task) {
            task.set('group', group);
            if (task.source) {
               task.set('displayName', task.source.unit_info.name);
               var stockpile = task.source['stonehearth:stockpile'];
               if (stockpile) {
                  var size = stockpile.size.x * stockpile.size.y;
                  var count = 0;
                  $.each(stockpile.stocked_items, function() { count += 1; });
                  var percent = Math.floor(count * 100 / size);
                  var percentFull = i18n.t('stonehearth:stockpile_percent_full', { percent: percent } );
                  task.set('subtext', percentFull);

                  if (stockpile.active) {
                     task.set('statusText', i18n.t('stonehearth:stockpile_active'));
                  } else {
                     task.set('statusText', i18n.t('stonehearth:stockpile_paused'));
                  }
               }
            }
            return true;
         });
         group.set('tasksArray', tasksArray);
         return true;
      });

      this.set('context.model.groupsArray', groupsArray);
    }.observes('context.model.groups'),

   _applyConfig: function() {
      var self = this;
      var pages = self.$('.tabPage');
      if (self._config && pages) {
         var tabId = self._config.selected_tab_id;
         var tab = self.$('#' + tabId);

         pages.hide();
         self.$('.tabButton').removeClass('active');
         if (tab && tab.length > 0) {
            tab.show();
            self.$('.tabButton[tab=' + tabId + ']').addClass('active');
         } else {
            self.$('.tabPage').first().show();
            self.$('.tabButton').first().show();
         }
      }
   },

   _addToolTips: function() {
      var self = this;
      Ember.run.scheduleOnce('afterRender', this, function() {
         self.$('.tabButton').tooltipster();
         self._applyConfig();
      });
   }.observes('context.model.groupsArray'),
});
