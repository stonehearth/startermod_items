App.StonehearthStartMenuView = App.View.extend({
   templateName: 'stonehearthStartMenu',
   //classNames: ['flex', 'fullScreen'],

   components: {
      "citizens" : {
         "*" : {
            "stonehearth:job" : {},
            "unit_info": {},
         }
      }
   },

   _foundjobs : {},

   menuActions: {
      harvest: function () {
         App.stonehearthClient.boxHarvestResources();
      },
      clear_item: function () {
         App.stonehearthClient.boxClearItem();
      },
      cancel_task: function () {
         App.stonehearthClient.boxCancelTask();
      },
      create_stockpile: function () {
         App.stonehearthClient.createStockpile();
      },
      create_farm : function () {
         App.stonehearthClient.createFarm();
      },
      create_trapping_grounds : function () {
         App.stonehearthClient.createTrappingGrounds();
      },
      create_pasture : function () {
         App.stonehearthClient.createPasture();
      },
      mining_ui : function () {
         App.stonehearthClient.mineCube();
      },
      mine_block : function () {
         App.stonehearthClient.mineBlock();
      },
      building_templates: function () {
         $(top).trigger('stonehearth_building_templates');
      },
      custom_building: function() {
         $(top).trigger('stonehearth_building_designer');
      },
      place_item: function () {
         $(top).trigger('stonehearth_place_item');
      },
      build_ladder: function () {
         App.stonehearthClient.buildLadder();
      },
      build_simple_room: function () {
         App.stonehearthClient.buildRoom();
      },
      loot_item : function () {
         App.stonehearthClient.boxLootItems();
      },
      town_menu: function() {
         App.stonehearthClient.showTownMenu();
      },
      citizen_manager: function() {
         App.stonehearthClient.showCitizenManager();
      },
      party_manager: function() {
         App.stonehearthClient.showPartyManager();
      },
      tasks_manager: function() {
         App.stonehearthClient.showTasksManager();
      },
      bulletin_manager: function() {
         App.bulletinBoard.toggleListView();
      },
      town_defense: function () {
         App.stonehearthClient.rallyWorkers();
      }
   },


   init: function() {
      this._super();
      var self = this;
   },

   didInsertElement: function() {
      this._super();

      var self = this;

      if (!this.$()) {
         return;
      }

      $('#startMenuTrigger').click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:trigger_click'} );
      });

      App.stonehearth.startMenu = self.$('#startMenu');

      $.get('/stonehearth/data/ui/start_menu.json')
         .done(function(json) {
            self._buildMenu(json);
            self._addHotkeys();
            self._tracePopulation();

            // hide menus that are in development
            radiant.call('radiant:get_config', 'show_in_progress_ui')
               .done(function(response) {
                  if (!response.show_in_progress_ui) {
                     self.$('#tasks_manager').hide();
                  }
               })

         });


      App.bulletinBoard.getTrace()
         .progress(function(result) {
            var bulletins = result.bulletins;

            if (bulletins && Object.keys(bulletins).length > 0) {
               //self.$('#bulletin_manager').pulse();
               self.$('#bulletin_manager').addClass('active');
            } else {
               self.$('#bulletin_manager').removeClass('active');
            }
         });

      /*
      $('#startMenu').on( 'mouseover', 'a', function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover'});
      });

      $('#startMenu').on( 'mousedown', 'li', function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click'});
      });
      */

   },

   destroy: function() {
      if (this._popTrace) {
         this._popTrace.destroy();
      }
   },

   _trackJobs: function(citizens) {
      // find all the jobs in the population
      radiant.each(citizens, function(_, citizen) {
         var job = citizen['stonehearth:job'];
         if (job) {
            // unlock this job in the menu
            self.$('#startMenu').stonehearthMenu('unlock', job.job_uri.alias);
         }
      });
   },

   _buildMenu : function(data) {
      var self = this;
      this.$('#startMenu').stonehearthMenu({ 
         data : data,
         click : function (id, nodeData) {
            self._onMenuClick(id, nodeData);
         }
      });

   },

   // create a trace to enable and disable menu items based on the jobs
   // in the population
   _tracePopulation: function() {
      var self = this;

      var components = {
         "citizens" : {
            "*" : {
               "stonehearth:job" : {
                  "job_uri" : {}
               }
            }
         }
      };

      self._popTrace = new RadiantTrace(App.population.getUri(), components, { bubbleNotificationsUp: true })
         .progress(function(pop) {
            self._trackJobs(pop.citizens);
         })
   },

   _onMenuClick: function(menuId, nodeData) {
      //radiant.keyboard.setFocus(this.$());
      var menuAction = this.menuActions[menuId];

      // do the menu action
      if (menuAction) {
         menuAction();
      }
   },


});

