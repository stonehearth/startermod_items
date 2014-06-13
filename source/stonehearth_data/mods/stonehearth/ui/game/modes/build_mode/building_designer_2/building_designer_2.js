App.StonehearthBuildingDesignerView2 = App.View.extend({
   templateName: 'buildingDesigner2',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', 'flex', "gui"],


   components: {
      'unit_info': {},
      'stonehearth:construction_progress' : {},
      'stonehearth:construction_data' : {
         'fabricator_entity' : {
            'stonehearth:fabricator' : {}      
         }
      },
      'stonehearth:fabricator' : {}
   },

   // building components is a list of all the components we should fetch about the 
   // building for the selected parts.  it's used to populate the building view
   blueprint_components: {
      'unit_info': {},
      'stonehearth:construction_progress' : {
         'building_entity' : {
            'unit_info' : {},
            'stonehearth:construction_progress': {},
         }
      }
   },

   actions: {
      selectDoodadTool: function(doodad) {
         App.stonehearthClient.addDoodad(doodad);
      },
      selectFloorBrushTool: function(floor) {
         App.stonehearthClient.buildFloor(floor);
      },
      selectWallMaterial: function(wall) {
         App.stonehearthClient.growWalls(this.get('context.building'), 'stonehearth:wooden_column', wall.uri);
      },
      selectRoofMaterial: function(wall) {
         //App.stonehearthClient.growWalls(this.get('context.building'), 'stonehearth:wooden_column', wall.uri);
      },
      selectFloorEraserTool: function() {
         App.stonehearthClient.eraseFloor();
      },
   },

   floorPatterns: [
      {
         category: 'Wooden Materials',
         items: [
            { name: 'Diagonal', portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb' },
            { name: 'Solid Light',    portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_light.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_solid_light.qb' },
            { name: 'Solid Dark',    portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_dark.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_solid_dark.qb' },
         ]         
      }
   ],

   wallPatterns: [
      {
         category: 'Wooden Materials',
         items: [
            { name: 'Wooden Wall', portrait: '/stonehearth/entities/build/wooden_wall/wooden_wall.png', uri: 'stonehearth:wooden_wall' },
            { name: 'Plastered Wooden Wall',    portrait: '/stonehearth/entities/build/wooden_wall/plastered_wooden_wall.png', uri: 'stonehearth:plastered_wooden_wall' },
         ]         
      }
   ],

   roofPatterns: [
      {
         category: 'Wooden Materials',
         items: [
         ]         
      }
   ],

   doodads: [
      {
         category: 'Windows',
         items: [
            { name: 'Wooden Window', portrait: '/stonehearth/entities/construction/wooden_window_frame/wooden_window_frame.png', uri:'stonehearth:wooden_window_frame' },
         ]
      },
      {
         category: 'Doors',
         items: [
            { name: 'Wooden Door', portrait: '/stonehearth/entities/construction/wooden_door/wooden_door.png', uri: 'stonehearth:wooden_door' },
         ]
      },
      {
         category: 'Decorations',
         items: [
            { name: 'Lamp', portrait: '/stonehearth/entities/construction/simple_wall_lantern/simple_wall_lantern.png', uri: 'stonehearth:simple_wall_lantern' },
         ]
      }
   ],

   init: function() {
      this._super();
      this.components['stonehearth:fabricator'].blueprint = this.blueprint_components;
      this.components['stonehearth:construction_data'].fabricator_entity['stonehearth:fabricator'].blueprint = this.blueprint_components;
      this.set('context.doodads', this.doodads);
      this.set('context.floorPatterns', this.floorPatterns);
      this.set('context.wallPatterns', this.wallPatterns);
   },

   // `uri` is a string that's valid to pass to radiant.trace()
   _contextUpdated: function() {
      var self = this;
      var context = this.get('context')
      var building_entity, blueprint_entity;

      var fabricator_component = context['stonehearth:fabricator'];
      var construction_data_component = context['stonehearth:construction_data'];
      var construnction_progress_component = context['stonehearth:construction_progress'];
      
      if (fabricator_component) {
         blueprint_entity = fabricator_component['blueprint'];
      } else if (construction_data_component) {
         blueprint_entity = construction_data_component.fabricator_entity['stonehearth:fabricator'].blueprint;
      } else if (construnction_progress_component) {
         building_entity = blueprint_entity = context
      }
      if (blueprint_entity && !building_entity) {
         building_entity = blueprint_entity['stonehearth:construction_progress']['building_entity'];         
      }
      self.set('context.building', building_entity);      
      if (building_entity) {
         self.set('context.building.active', building_entity['stonehearth:construction_progress'].active);
      }
      self.set('context.blueprint', blueprint_entity);
      this.set('context.doodads', this.doodads);
      this.set('context.floorPatterns', this.floorPatterns);
      this.set('context.wallPatterns', this.wallPatterns);
      self._updateControls();
   }.observes('context'),

   _updateControls: function() {   
   },

   _activeUpdated: function() {
      var building = this.get('context.building');
      var value = false;
      if (!building) {
         return;
      }
      value = building['stonehearth:construction_progress'].active;
      this.set('context.building.active', value)
   }.observes('context.building.stonehearth:construction_progress.active'),

   didInsertElement: function() {
      var self = this;
      this._super();

      this.$('.tabButton').click(function() {
         App.stonehearthClient.deactivateAllTools();
         
         var tab = $(this).attr('tab');

         var currentTab = self.$('#selectedBuildingWindow .tabPage');
         if (currentTab) {
            currentTab.hide();
         }
         var nextTab = self.$('#selectedBuildingWindow #' + tab)
         if (nextTab) {
            nextTab.show();
         }         
      });

      this.$('.roofTool').click(function() {
         App.stonehearthClient.growRoof(self.get('context.building'));
      });

      this.$('#startBuilding').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:submenu_select' );
         var building_entity = self.get('context.building');
         if (building_entity) {
            var value = !self.get('context.building.active')
            //stonehearth.server.build.set_building_active(building_entity.__self, value);
            radiant.call('stonehearth:set_building_active', building_entity.__self, value)
         }
      });

      this.$('#removeBuilding').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:trash' );
         var building_entity = self.get('context.building');
         if (building_entity) {
            //stonehearth.server.build.remove_building(building_entity.__self, value);
            radiant.call('stonehearth:set_building_teardown', building_entity.__self, true)
         }
      });
   },

   _onEntitySelected: function(e) {
      var self = this
      
      self._selectedEntity = e.selected_entity      
      if (!self._selectedEntity) {
         return;
      }

      // nuke the old trace
      if (self.selectedEntityTrace) {
         self.selectedEntityTrace.destroy();
      }

      // trace the properties so we can tell if we need to popup the properties window for the object
      self.selectedEntityTrace = radiant.trace(this._selectedEntity)
         .progress(function(result) {
            self._examineEntity(result);
         })
         .fail(function(e) {
            console.log(e);
         });
   },

});
