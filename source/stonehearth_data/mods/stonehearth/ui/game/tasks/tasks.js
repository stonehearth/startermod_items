App.StonehearthTasksView = App.View.extend({
	templateName: 'tasks',
   classNames: ['exclusive'],
   closeOnEsc: true,

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Tasks');
      self.get('context.groups');

      // Create the task group buttons
      radiant.trace('stonehearth:data:player_task_groups')
         .progress(function(json) {
            var playerTaskGroups = json.task_groups;

            // Get the task model
            radiant.call('stonehearth:get_tasks')
                     .done(function(response) {
                        var groups = [];
                        radiant.each(response.groups, function(key, group_uri) {
                           var name = playerTaskGroups[key].name;
                           var group = {
                              key: name,
                              displayName: i18n.t(name),
                              taskIcon: '/stonehearth/ui/game/tasks/images/tasks/' + name + '.png',
                              tabButtonIcon: '/stonehearth/ui/game/tasks/images/buttons/' + name + '.png',
                              uri: group_uri,
                           }
                           groups.push(group);
                        });
                        self.set('context.groups', groups);
                     });
         })
         .fail(function(o) {
            console.log(o);
         });

      // select the last selected tab
      radiant.call('radiant:get_config', 'stonehearth_tasks_view')
         .done(function(response) {
            self._config = response.stonehearth_tasks_view || {};
            self._applyConfig();
         });


   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();   
         this._trace = null;
      }   
      this._super();
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


   _applyConfig: function() {
      var self = this;
      var pages = self.$('.tabPage');
      if (self._config && pages && pages.length > 0) {
         var tabId = self._config.selected_tab_id;
         var tabButton = self.$('.tabButton[tab=' + tabId + ']')
         var tab = self.$('#' + tabId);


         pages.hide();
         self.$('.tabButton').removeClass('active');
         if (tabButton && tabButton.length > 0) {
            tabButton.addClass('active');
            tab.show();
         } else {
            self.$('.tabButton').first().addClass('active');
            pages().first().show();
         }
      }
   },

   _addToolTips: function() {
      var self = this;
      Ember.run.scheduleOnce('afterRender', this, function() {
         self.$('.tabButton').tooltipster();
         self._applyConfig();
      });
   }.observes('context.groups').on('init'),
});

App.StonehearthTasksGroupView = App.View.extend({
   templateName: 'taskGroup',

   _components: {
      "tasks" : {
         "*" : {
            "source" : {
               "unit_info" : {},
               "stonehearth:stockpile" : {}
            }
         }
      }
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


   init: function() {
      var self = this;
      this._super();


      var uri = self.get('context.uri');
      this._trace = new StonehearthDataTrace(uri, self._components)
                           .progress(function(model) {
                              self.set('context.model', model);
                           });
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();   
         this._trace = null;
      }   
      this._super();
   },

   _rebuildGroupsArray: function() {
      var self = this;
      if (!self.get('context')) {
         return;
      }

      var tasks = this.get('context.model.tasks');
      var tasksArray = radiant.map_to_array(tasks, function(key, task) {
         if (task.source) {
            task.set('displayName', task.source.unit_info.name);
            var stockpile = task.source['stonehearth:stockpile'];
            if (stockpile) {
               var size = stockpile.size.x * stockpile.size.y;
               var count = 0;
               radiant.each(stockpile.stocked_items, function() { count += 1; });
               var percent = Math.floor(count * 100 / size);
               var percentFull = i18n.t('stonehearth:stockpile_percent_full', { percent: percent } );
               task.set('subtext', percentFull);

               task.addObserver('stonehearth:stockpile', self, function() {
                  if (stockpile.active) {
                     task.set('statusText', i18n.t('stonehearth:stockpile_active'));
                  } else {
                     task.set('statusText', i18n.t('stonehearth:stockpile_paused'));
                  }
               });
            }
         }
      });
      this.set('context.tasks', tasksArray);
   }.observes('context.model').on('init'),
});
