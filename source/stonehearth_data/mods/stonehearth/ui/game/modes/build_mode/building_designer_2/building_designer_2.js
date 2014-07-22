App.StonehearthBuildingDesignerTools = App.View.extend({
   templateName: 'buildingDesignerTools',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', 'flex', "gui"],
   uriProperty: 'context.selection',

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
      'stonehearth:construction_data' : {},
      'stonehearth:construction_progress' : {
         'building_entity' : {
            'unit_info' : {},
            'stonehearth:construction_data' : {},
            'stonehearth:construction_progress': {},
         }
      }
   },

   init: function() {
      var self = this;

      this._super();
      this.components['stonehearth:fabricator'].blueprint = this.blueprint_components;
      this.components['stonehearth:construction_data'].fabricator_entity['stonehearth:fabricator'].blueprint = this.blueprint_components;

   },   

   floorPatterns: [
      {
         category: 'Wooden Materials',
         items: [
            { id: 'light', name: 'Solid Light',    portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_light.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_solid_light.qb' },
            { id: 'dark', name: 'Solid Dark',    portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_dark.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_solid_dark.qb' },
            { id: 'diagonal', name: 'Diagonal', portrait: '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.png', brush:'/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb' },
         ]         
      }
   ],

   wallPatterns: [
      {
         category: 'Wooden Materials',
         items: [
            { name: 'Plastered Wooden Wall',    portrait: '/stonehearth/entities/build/wooden_wall/plastered_wooden_wall.png', brush: 'stonehearth:plastered_wooden_wall' },
            { name: 'Wooden Wall', portrait: '/stonehearth/entities/build/wooden_wall/wooden_wall.png', brush: 'stonehearth:wooden_wall' },
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
         category: 'Doors',
         items: [
            { name: 'Wooden Door', portrait: '/stonehearth/entities/construction/wooden_door/wooden_door.png', brush: 'stonehearth:wooden_door' },
         ]
      },
      {
         category: 'Windows',
         items: [
            { name: 'Wooden Window', portrait: '/stonehearth/entities/construction/wooden_window_frame/wooden_window_frame.png', brush:'stonehearth:wooden_window_frame' },
         ]
      },
      {
         category: 'Decorations',
         items: [
            { name: 'Lamp', portrait: '/stonehearth/entities/construction/lantern_wooden_box/lantern_wooden_box.png', brush: 'stonehearth:lantern_wooden_box' },
         ]
      }
   ],


   // Save the state of the dialog int the 'stonehearth:building_designer' key.
   _saveState: function() {
      radiant.call('stonehearth:save_browser_object', 'stonehearth:building_designer', this._state);
   },

   _buildMaterialPalette: function(materials, materialClassName) {
      var palette = $('<div>')
                     .addClass('brushPalette')
                     .addClass('section');

      // for each category
      var index = 0;
      $.each(materials, function(i, category) {
         // for each material
         $.each(category.items, function(k, material) {
            var brush = $('<img>')
                           .attr('brush', material.brush)
                           .attr('src', material.portrait)
                           .attr('title', material.name)
                           .attr('index', index)
                           .addClass(materialClassName);

            palette.append(brush);
            index += 1;
         });
      });

      var el = $('<div>')
                  .append('<h2>Materials</h2>')
                  .append(palette);

      return el;
   },
      // grow roof button
   _getRoofOptionsFromElement : function(selector, options) {
      var self = this;
      options.nine_grid_gradiant = []

      self.$(selector + ' .roofDiagramButton.active').each(function() {
         if($(this).is(':visible')) {
            options.nine_grid_gradiant.push($(this).attr('gradient'));
         }
      })
   },

   _updateGrowRoofOptions : function() {
      App.stonehearthClient.setGrowRoofOptions(this._state.growRoofOptions);
   },

   didInsertElement: function() {
      var self = this;

      // build material palettes
      this.$('#floorToolTab').append(this._buildMaterialPalette(this.floorPatterns, 'floorMaterial'));
      this.$('#wallToolTab').append(this._buildMaterialPalette(this.wallPatterns, 'wallMaterial'));
      this.$('#doodadToolTab').append(this._buildMaterialPalette(this.doodads, 'doodadMaterial'));

      // tab buttons and pages
      this.$('.tabButton').click(function() {
         App.stonehearthClient.deactivateAllTools();
         var tabId = $(this).attr('tab');
         var tab = self.$('#' + tabId);

         self.$('.tabButton').removeClass('active');
         $(this).addClass('active');

         // restore the last used tool for the tab
         var activeTool = self._state[tabId + "ActiveTool"];
         if (activeTool) {
            tab.find(activeTool).click();   
         }

         // show the correct tab page
         self.$('.tabPage').hide();
         tab.show();

         self._state.activeTabId = tabId;
         self._saveState();
      });

      // tools
      this.$('.toolButton').click(function() {
         var tool = $(this);
         var toolActive = tool.hasClass('active');
         var toolSaveState = null;

         // change display for all the other tool buttons
         self.$('.toolButton').removeClass('active');         
         
         // toggle the tool based on its old state
         if (toolActive) {
            // was active, deactivate it
            toolSaveState = null;
            App.stonehearthClient.deactivateAllTools();
         } else {
            // was not active, activate it
            tool.addClass('active');
            toolSaveState = '#' + tool.attr('id');
         }

         var tabId = tool.parents('.tabPage').attr('id');
         self._state[tabId + "ActiveTool"] = toolSaveState;
         self._saveState();
      });

      // undo/redoo tool
      this.$('#undoTool').click(function() {
         App.stonehearthClient.undo();
      });

      // floor materials
      this.$('.floorMaterial').click(function() {
         self.$('.floorMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.floorMaterial = $(this).attr('index');
         self._saveState();

         self._updateTool(self.$('#drawFloorTool'));
      })

      // draw floor tool
      this.$('#drawFloorTool').click(function() {
         if($(this).hasClass('active')) {
            var brush = self.$('#floorToolTab .floorMaterial.selected').attr('brush');
            App.stonehearthClient.buildFloor(brush);
            self._activeTool = $(this);            
         }
      });

      this.$('#eraseFloorTool').click(function() {
         if($(this).hasClass('active')) {
            App.stonehearthClient.eraseFloor();
         }
      });
      

      // wall tool tab
      this.$('#wallToolTab .wallMaterial').click(function() {
         // select the tool
         self.$('#wallToolTab .wallMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.wallMaterial = $(this).attr('index');
         self._saveState();

         self._updateTool();
      })

      // draw wall tool
      this.$('#drawWallTool').click(function() {
         if($(this).hasClass('active')) {
            var wallUri = self.$('#wallToolTab .wallMaterial.selected').attr('brush');
            App.stonehearthClient.buildWall('stonehearth:wooden_column', wallUri);

            self._activeTool = $(this);
         }
      });

      // grow walls tool
      this.$('#growWallsTool').click(function() {
         if($(this).hasClass('active')) {
            var wallUri = self.$('#wallToolTab .wallMaterial.selected').attr('brush');
            App.stonehearthClient.growWalls('stonehearth:wooden_column', wallUri);
         }
      });

      this.$('#growRoofTool').click(function() {
         if($(this).hasClass('active')) {
            App.stonehearthClient.growRoof();
         }
      })

      // roof numeric inputs like maxheight and slope

      // common behavior between options tab and edit tab
      var roofNumericInputChange = function(el, options) {
         var numericInput = el;
         if (numericInput.attr('id') == 'inputMaxRoofHeight') {
            options.nine_grid_max_height = numericInput.val();
         } else if (numericInput.attr('id') == 'inputMaxRoofSlope') {
            options.nine_grid_slope = numericInput.val();
         }
      };

      // options tab
      this.$('#roofToolTab .roofNumericInput').change(function() {
         roofNumericInputChange($(this), self._state.growRoofOptions);
         self._updateGrowRoofOptions();
         self._saveState();         
      });

      // edit tab
      this.$('#editToolTab .roofNumericInput').change(function() {
         var constructionData = self.get('blueprint.stonehearth:construction_data');
         if (constructionData) {
            var options = {}
            roofNumericInputChange($(this), options);
            radiant.call_obj(constructionData, 'apply_options_command', options);
         }
      });


      // roof diagram buttons

      // common behavior between options tab and edit tab
      var roofDiagramButtonClick = function(el) {
         el.toggleClass('active');
      }

      // options tab
      this.$('#roofToolTab .roofDiagramButton').click(function() {
         roofDiagramButtonClick($(this));
         self._getRoofOptionsFromElement('#roofToolTab', self._state.growRoofOptions);
         self._updateGrowRoofOptions();
         self._saveState();
      });

      // edit tab
      this.$('#editToolTab .roofDiagramButton').click(function() {
         roofDiagramButtonClick($(this));

         var constructionData = self.get('blueprint.stonehearth:construction_data');
         if (constructionData) {
            var options = {}
            self._getRoofOptionsFromElement('#editToolTab', options);
            radiant.call_obj(constructionData, 'apply_options_command', options);
         }
      });

      // doodad material
      this.$('.doodadMaterial').click(function() {
         self.$('.doodadMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.doodadMaterial = $(this).attr('index');
         self._saveState();

         self._updateTool(self.$('#drawDoodadTool'));
      })

      // draw doodad tool
      this.$('#drawDoodadTool').click(function() {
         if($(this).hasClass('active')) {
            var uri = self.$('#doodadToolTab .doodadMaterial.selected').attr('brush');
            App.stonehearthClient.addDoodad(uri);

            self._activeTool = $(this);
         }
      });

      // edit tab
      $('#materialPicker').on( 'click', '.wallMaterial', function() {
         var wallUri = $(this).attr('brush');
         var blueprint = self.get('blueprint.__self');
         App.stonehearthClient.replaceStructure(blueprint, wallUri); 
      });

      $('#editToolTab #roofEditor')

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

      // restore the state of the dialog from the last time it was invoked and 
      radiant.call('stonehearth:load_browser_object', 'stonehearth:building_designer')
         .done(function(o) {
            self._state = o.value || {};
            if (!self._state.activeTabId) {
               self._state.activeTabId = 'floorToolButton';
            }
            if (!self._state.floorMaterial) {
               self._state.floorMaterial = 0;
            }
            if (!self._state.wallMaterial) {
               self._state.wallMaterial = 0;
            }
            if (!self._state.doodadMaterial) {
               self._state.doodadMaterial = 0;
            }
            if (!self._state.growRoofOptions) {
               self._state.growRoofOptions = {
                  nine_grid_gradiant: [ 'left', 'right' ],
                  nine_grid_max_height: 4,
                  nine_grid_slope: 1,
               }
            }
            self._applyControlState();
         });
   },

   // Make the roof gradiant picker match the specified gradiant.  gradiant is an
   // array of the 'left', 'right', 'front', and 'back' flags.
   _applyRoofGradiantControlState : function(selector, options) {
      var self = this;

      self.$(selector + ' .roofDiagramButton').removeClass('active');
      $.each(options.nine_grid_gradiant || [], function(_, dir) {
         self.$(selector + ' .roofDiagramButton[gradient="' + dir + '"]').addClass('active');
      });

      $(selector + ' #inputMaxRoofHeight').val(options.nine_grid_max_height || 4);
      $(selector + ' #inputMaxRoofSlope').val(options.nine_grid_slope || 1);
   },

   _applyControlState: function() {
      var self = this;
      if (self._state) {
         // select default materials
         $(self.$('#floorToolTab .floorMaterial')[self._state.floorMaterial]).addClass('selected');
         $(self.$('#wallToolTab .wallMaterial')[self._state.wallMaterial]).addClass('selected');
         $(self.$('#doodadToolTab .doodadMaterial')[self._state.doodadMaterial]).addClass('selected');

         // most recently selected tab
         self.$("[tab='" + self._state.activeTabId + "']").click();

         // gradiant on the grow roof control
         self._applyRoofGradiantControlState('#roofToolTab', self._state.growRoofOptions);
         self._updateGrowRoofOptions()
      }
   },

   _updateTool: function(toolElement) {
      var self = this;

      // reactivate the active tool with this new material
      if (self._activeTool) {
         self._activeTool.click();
      } else if (toolElement) {
         toolElement.click();
      }
   },

   _updateSelection: function(building) {
      var self = this;
      var building = this.get(this.uriProperty);
      var building_entity, blueprint_entity;

      if (building) {
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
         self.set('blueprint', blueprint_entity);

         if (building_entity) {
            self.set('building.active', building_entity['stonehearth:construction_progress'].active);
         }        
      }

      self._updateControls();
   }.observes('context.selection'),

   _updateControls: function() {
      var self = this;

      if(!self.$) {
         return;
      }

      var building_entity = this.get('building');
      var blueprint_entity = this.get('blueprint');

      if (building_entity) {
         self.$('#selectedBuildingWindow').show();
      } else {
         self.$('#selectedBuildingWindow').hide();
      }

      if (blueprint_entity) {
         var materialPicker = self.$('#materialPicker');
         var constructionData = this.get('blueprint.stonehearth:construction_data')
         var type = constructionData.type;

         // Reset the current selection
         materialPicker.empty();
         self.$('#editToolTab #roofEditor').hide();

         // Add the editor for the current type
         var materials;
         if (type == 'wall') {           
            materials = this._buildMaterialPalette(this.wallPatterns, 'wallMaterial');
         } else if (type == 'roof') {
            self.$('#editToolTab #roofEditor').show();
            self._applyRoofGradiantControlState('#roofEditor', constructionData);
         }
         materialPicker.append(materials);            
      }
   }
});
