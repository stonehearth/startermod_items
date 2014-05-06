App.StonehearthStartMenuView = App.View.extend({
   templateName: 'stonehearthStartMenu',
   //classNames: ['flex', 'fullScreen'],

   components: {
      "citizens" : {
         "*" : {
            "stonehearth:profession" : {},
            "unit_info": {},
         }
      }
   },

   _foundProfessions : {},

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
      design_building: function () {
         $(top).trigger('stonehearth_design_building');
      },
      build_wall: function () {
         App.stonehearthClient.buildWall();
      },
      build_simple_room: function () {
         App.stonehearthClient.buildRoom();
      },
      citizen_manager: function() {
         App.stonehearthClient.showCitizenManager();
      },
      crafter_manager: function() {
         App.stonehearthClient.showCrafterManager();
      },
      placeItem: function () {
         $(top).trigger('radiant_show_placement_menu');
      }
   },


   init: function() {
      this._super();
      var self = this;
   },

   didInsertElement: function() {
      var self = this;

      if (!this.$()) {
         return;
      }

      $('#startMenuTrigger').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:trigger_click' );
      });

      App.stonehearth.startMenu = self.$('#startMenu');

      self.$('design_building').hide();

      /*
      radiant.call('stonehearth:get_start_menu_data')
         .done(function (obj) {
            radiant.trace(obj.datastore)
            .progress(function(result) {
                  self._buildMenu(result);
               })
               .fail(function(e) {
                  console.log(e);
               });
         });
      */

      $.get('/stonehearth/data/ui/start_menu.json')
         .done(function(json) {
            self._buildMenu(json);
         });

      radiant.call('stonehearth:get_population')
         .done(function(response){
            // xxx: setting 'uri' causes the view to re-reinitialize itself!
            //self.set('uri', response.population);
         });

      /*
      $('#startMenu').on( 'mouseover', 'a', function() {
         radiant.call('radiant:play_sound', 'stoneheardh:sounds:ui:action_hover');
      });

      $('#startMenu').on( 'mousedown', 'li', function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click');
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

   _onCitizenProfessionChanged: function() {
      var self = this;
      var citizensArray = this.get('context.citizensArray');

      self._foundProfessions = {}
      $.each(citizensArray, function(i, citizen) {
         var profession_uri = citizen['stonehearth:profession']['profession_uri'];
         self._foundProfessions[profession_uri] = true;
      });

      self._updateMenuLocks();
   }.observes('context.citizensArray.@each.stonehearth:profession'),

   _updateMenuLocks: function() {
      var self = this;

      if (!self.$()) {
         return;
      }

      $.each(this._foundProfessions, function(profession, _) {
         $( ".selector" ).progressbar({ disabled: true });
         self.$('#startMenu').stonehearthMenu('unlock', profession);
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

