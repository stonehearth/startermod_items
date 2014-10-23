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
      harvest_menu: function () {
         App.stonehearthClient.boxHarvestResources();
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
      dig_down : function () {
         App.stonehearthClient.digDown();
      },
      dig_out : function () {
         App.stonehearthClient.digOut();
      },
      building_templates: function () {
         $(top).trigger('stonehearth_building_templates');
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
      town_menu: function() {
         App.stonehearthClient.showTownMenu();
      },
      citizen_manager: function() {
         App.stonehearthClient.showCitizenManager();
      },
      crafter_manager: function() {
         App.stonehearthClient.showCrafterManager();
      },
      red_alert: function () {
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
         });


      radiant.call('stonehearth:get_population')
         .done(function(response){
            // xxx: setting 'uri' causes the view to re-reinitialize itself!
            //self.set('uri', response.population);
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

   _onCitizensChanged: function() {
      var vals = [];
      var citizenMap = this.get('context.citizens');
     
      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k != "__self" && citizenMap.hasOwnProperty(k)) {
               vals.push(v);
            }
         });
      }

      this.set('context.citizensArray', vals);
    }.observes('context.citizens.[]'),

   _onCitizenjobChanged: function() {
      var self = this;
      var citizensArray = this.get('context.citizensArray');

      self._foundjobs = {}
      $.each(citizensArray, function(i, citizen) {
         var job_uri = citizen['stonehearth:job']['job_uri'];
         self._foundjobs[job_uri] = true;
      });

      self._updateMenuLocks();
   }.observes('context.citizensArray.@each.stonehearth:job'),

   _updateMenuLocks: function() {
      var self = this;

      if (!self.$()) {
         return;
      }

      $.each(this._foundjobs, function(job, _) {
         $( ".selector" ).progressbar({ disabled: true });
         self.$('#startMenu').stonehearthMenu('unlock', job);
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

   _onMenuClick: function(menuId, nodeData) {
      //radiant.keyboard.setFocus(this.$());
      var menuAction = this.menuActions[menuId];

      // do the menu action
      if (menuAction) {
         menuAction();
      }
   },


});

