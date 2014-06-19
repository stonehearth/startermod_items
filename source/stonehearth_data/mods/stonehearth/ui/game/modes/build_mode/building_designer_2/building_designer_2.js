App.StonehearthBuildingDesignerTools = App.View.extend({
   templateName: 'buildingDesignerTools',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', 'flex', "gui"],
   uriProperty: 'selection',

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

   init: function() {
      this._super();
      this.components['stonehearth:fabricator'].blueprint = this.blueprint_components;
      this.components['stonehearth:construction_data'].fabricator_entity['stonehearth:fabricator'].blueprint = this.blueprint_components;
      this.set('foobar', 'foobar');
   },

   floorPatterns: [
      {
         category: 'Wooden Materials',
         items: [
            { id: 'diagonal', name: 'Diagonal', portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb' },
            { id: 'light', name: 'Solid Light',    portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_light.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_solid_light.qb' },
            { id: 'dark', name: 'Solid Dark',    portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_dark.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_solid_dark.qb' },
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

   didInsertElement: function() {
      var self = this;

      // tab buttons
      this.$('.tabButton').click(function() {
         App.stonehearthClient.deactivateAllTools();
         
         var tab = $(this).attr('tab');
         self.$('.tabPage').hide();
         self.$('#' + tab).show();
      });

      // undo/redoo tool
      this.$('#undoTool').click(function() {
         App.stonehearthClient.undo();
      });

      // floor materials
      this.$('.floorMaterial').click(function() {
         self.$('.floorMaterial').removeClass('selected');
         $(this).addClass('selected');
      })

      // draw floor tool
      this.$('#drawFloorTool').click(function() {
         var brush = self.$('#floorToolTab .floorMaterial.selected').attr('brush');
         App.stonehearthClient.buildFloor(brush);
      });

      this.$('#eraseFloorTool').click(function() {
         App.stonehearthClient.eraseFloor();
      });
      

      // wall tool tab
      this.$('#wallToolTab .wallMaterial').click(function() {
         // select the tool
         self.$('#wallToolTab .wallMaterial').removeClass('selected');
         $(this).addClass('selected');
      })

      // draw wall tool
      this.$('#drawWallTool').click(function() {
         var wallUri = self.$('#wallToolTab .wallMaterial.selected').attr('uri');
         App.stonehearthClient.buildWall('stonehearth:wooden_column', wallUri);
      });

      // grow walls tool
      this.$('#growWallsTool').click(function() {
         
         var building = self.get('building');
         var wallUri = self.$('#wallToolTab .wallMaterial.selected').attr('uri');
         App.stonehearthClient.growWalls(building, 'stonehearth:wooden_column', wallUri);
      });

      // grow walls wizard
      this.$('#showGrowWallsWizard').click(function() {
         
         self.$('#growWallsWizard').show();
      });

      this.$('#closeGrowWallsWizard').click(function() {
         self.$('#growWallsWizard').fadeOut();
      });

      this.$('#growWallsWizard .wallMaterial').click(function() {
         // select the tool
         self.$('#growWallsWizard .wallMaterial').removeClass('selected');
         $(this).addClass('selected');         
      });


      this.$('#growWallsButton').click(function() {
         var building = self.get('building');
         var wallUri = $('#growWallsWizard .wallMaterial.selected').attr('uri');

         App.stonehearthClient.growWalls(building, 'stonehearth:wooden_column', wallUri);
         self.$('#growWallsWizard').hide();
      });

      
      // grow roof button
      this.$('#growRoofTool').click(function() {
         var building = self.get('building');
         // todo, open a wizard
         App.stonehearthClient.growRoof(building);
      })

      // doodad tool
      this.$('.doodadTool').click(function() {
         // select the tool
         self.$('.doodadTool').removeClass('selected');
         $(this).addClass('selected');
         
         // activate the tool
         var uri = $(this).attr('uri');
         App.stonehearthClient.addDoodad(uri);
      })

      // building buttons
      this.$('#startBuilding').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:submenu_select' );
         var building_entity = self.get('building');
         if (building_entity) {
            var value = !self.get('building.active')
            //stonehearth.server.build.set_building_active(building_entity.__self, value);
            radiant.call('stonehearth:set_building_active', building_entity.__self, value)
         }
      });

      this.$('#removeBuilding').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:trash' );
         var building_entity = self.get('building');
         if (building_entity) {
            //stonehearth.server.build.remove_building(building_entity.__self, value);
            radiant.call('stonehearth:set_building_teardown', building_entity.__self, true)
         }
      });

      // select default materials
      $(self.$('#floorToolTab .floorMaterial')[0]).addClass('selected');
      $(self.$('#wallToolTab .wallMaterial')[0]).addClass('selected');
   },

   _updateSelection: function(building) {
      var self = this;
      var building = this.get(this.uriProperty);
      var building_entity, blueprint_entity;

      var fabricator_component = building['stonehearth:fabricator'];
      var construction_data_component = building['stonehearth:construction_data'];
      var construnction_progress_component = building['stonehearth:construction_progress'];
      
      if (fabricator_component) {
         blueprint_entity = fabricator_component['blueprint'];
      } else if (construction_data_component) {
         blueprint_entity = construction_data_component.fabricator_entity['stonehearth:fabricator'].blueprint;
      } else if (construnction_progress_component) {
         building_entity = blueprint_entity = building
      }
      if (blueprint_entity && !building_entity) {
         building_entity = blueprint_entity['stonehearth:construction_progress']['building_entity'];         
      }

      self.set('building', building_entity);

      if (building_entity) {
         self.set('building.active', building_entity['stonehearth:construction_progress'].active);
      }

      self._updateControls();
   }.observes('selection'),

   _updateControls: function() {

      if(!this.$) {
         return;
      }

      var building_entity = this.get('building');

      if (building_entity) {
         self.$('#selectedBuildingWindow').show();
      } else {
         self.$('#selectedBuildingWindow').hide();
      }
   }
});
