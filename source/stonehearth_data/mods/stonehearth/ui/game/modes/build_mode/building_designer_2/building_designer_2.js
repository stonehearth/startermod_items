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

   _selectActiveMaterial: function(el) {
      var self = this;

      var currentMaterial = self.get('blueprint.uri');

      if (currentMaterial) {
         var selector = '[brush="' + currentMaterial + '"]';
         var palette = el.find('.brushPalette');

         var match = palette.find(selector).addClass('selected');

         if (match.length > 0) {
            palette.children().removeClass('selected');
            match.addClass('selected');
         }
      }
      
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
      this._super();
      self._state = {};

      $.get('/stonehearth/data/build/building_parts.json')
         .done(function(json) {
            self.buildingParts = json;
            self.$('#floorToolTab').append(self._buildMaterialPalette(self.buildingParts.floorPatterns, 'floorMaterial'));
            self.$('#wallToolTab').append(self._buildMaterialPalette(self.buildingParts.wallPatterns, 'wallMaterial'));
            self.$('.roofMaterialsContainer').append(self._buildMaterialPalette(self.buildingParts.roofPatterns, 'roofMaterial'));
            self.$('#doodadToolTab').append(self._buildMaterialPalette(self.buildingParts.doodads, 'doodadMaterial'));

            self._addEventHandlers();
            self._restoreUiState();
         });
   },

   _addEventHandlers: function() {
      var self = this;
      
      // tab buttons and pages
      this.$('.tabButton').click(function() {
         var tabId = $(this).attr('tab');
         var tab = self.$('#' + tabId);

         if (!tabId) {
            return;
         }

         // If a new tab is being selected, deactivate the old tools
         var clickedTab = $(this).attr('id');
         var currentActiveTab = self.$('.tabButton.active').attr('id');
         if(clickedTab != currentActiveTab) {
            App.stonehearthClient.deactivateAllTools();   
         }
         
         // style the tabs to reflect the new active tab
         self.$('.tabButton').removeClass('active');
         $(this).addClass('active');

         // if the selected entity is not applicable to this tab, deselect the entity
         var buildingType = $(this).attr('buildingType');
         var constructionData = self.get('blueprint.stonehearth:construction_data')

         if (constructionData && buildingType != constructionData.type) {
            radiant.call('stonehearth:select_entity', null);   
         }
         
         // restore the last used tool for the tab
         var activeTool = self._state[tabId + "ActiveTool"];
         if (activeTool) {
            // disable this, because it messes with the ability to just click around the parts of the building.
            // For example, you click a wall, the grow walls tool activates, and you can't click around anymore
            //tab.find(activeTool).click();   
         }

         // update the material in the tab to reflect the selection
         self._selectActiveMaterial(tab);

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

      // draw floor tool
      var doDrawFloor = function() {
         if(self.$('#drawFloorTool').hasClass('active')) {
            var brush = self.$('#floorToolTab .floorMaterial.selected').attr('brush');
            App.stonehearthClient.buildFloor(brush);
         }
      }

      this.$('#drawFloorTool').click(function() {
         doDrawFloor();
      });

      this.$('#eraseFloorTool').click(function() {
         if($(this).hasClass('active')) {
            App.stonehearthClient.eraseFloor();
         }
      });

      // floor materials
      this.$('.floorMaterial').click(function() {
         self.$('.floorMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.floorMaterial = $(this).attr('index');
         self._saveState();

         // reactivate the floor tool with the new material
         doDrawFloor();
      })      

      // wall tool tab

      // draw wall tool
      var doDrawWall = function() {
         if(self.$('#drawWallTool').hasClass('active')) {
            var wallUri = self.$('#wallToolTab .wallMaterial.selected').attr('brush');
            App.stonehearthClient.buildWall('stonehearth:wooden_column', wallUri);
         }
      }

      var doGrowWalls = function() {
         if(self.$('#growWallsTool').hasClass('active')) {
            var wallUri = self.$('#wallToolTab .wallMaterial.selected').attr('brush');
            App.stonehearthClient.growWalls('stonehearth:wooden_column', wallUri);
         }
      }

      this.$('#drawWallTool').click(function() {
         doDrawWall();
      });

      // grow walls tool
      this.$('#growWallsTool').click(function() {
         doGrowWalls();
      });

      // wall materials
      this.$('#wallToolTab .wallMaterial').click(function() {
         // select the clicked material
         self.$('#wallToolTab .wallMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.wallMaterial = $(this).attr('index');
         self._saveState();

         // update the selected building part, if there is one
         var wallUri = $(this).attr('brush');
         var blueprint = self.get('blueprint');
         if (blueprint) {
            App.stonehearthClient.replaceStructure(blueprint, wallUri);
         }

         // reactivate the active tool with the new material. 
         doDrawWall();
         doGrowWalls();
      })

      // roof tab
      var doGrowRoof = function() {
         if(self.$('#growRoofTool').hasClass('active')) {
            var roofUri = self.$('#roofToolTab .roofMaterial.selected').attr('brush');
            App.stonehearthClient.growRoof(roofUri);
         }
      }

      this.$('#growRoofTool').click(function() {
         doGrowRoof();
      })

      this.$('#roofToolTab .roofMaterial').click(function() {
         // select the clicked material
         self.$('#roofToolTab .roofMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.roofMaterial = $(this).attr('index');
         self._saveState();

         // update the selected building part, if there is one
         // XXX - ack, gotta fix this
         /*
         var roofUri = $(this).attr('brush');
         var blueprint = self.get('blueprint');
         if (blueprint) {
            App.stonehearthClient.replaceStructure(blueprint, roofUri);
         }
         */

         // reactivate the active tool with the new material. 
         doGrowRoof();
      })

      this.$('#roofToolTab .roofNumericInput').change(function() {
         // update the options for future roofs
         var numericInput = $(this);
         var options = self._state.growRoofOptions;
         if (numericInput.attr('id') == 'inputMaxRoofHeight') {
            options.nine_grid_max_height = parseInt(numericInput.val());
         } else if (numericInput.attr('id') == 'inputMaxRoofSlope') {
            options.nine_grid_slope = parseInt(numericInput.val());
         }

         self._updateGrowRoofOptions();
         self._saveState();

         // if a roof is selected, change it too
         var blueprint = self.get('blueprint');
         if (blueprint) {
            App.stonehearthClient.applyConstructionDataOptions(blueprint, options);
         }

      });

      // roof slope buttons
      this.$('#roofToolTab .roofDiagramButton').click(function() {
         // update the options for future roofs
         $(this).toggleClass('active');
         self._getRoofOptionsFromElement('#roofToolTab', self._state.growRoofOptions);
         self._updateGrowRoofOptions();
         self._saveState();

         // if a roof is selected, change it too
         var blueprint = self.get('blueprint');
         if (blueprint) {
            App.stonehearthClient.applyConstructionDataOptions(blueprint, self._state.growRoofOptions);
         }

      });


      // doodad material
      this.$('.doodadMaterial').click(function() {
         self.$('.doodadMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.doodadMaterial = $(this).attr('index');
         self._saveState();
      })

      // draw doodad tool
      this.$('#drawDoodadTool').click(function() {
         if($(this).hasClass('active')) {
            var uri = self.$('#doodadToolTab .doodadMaterial.selected').attr('brush');
            App.stonehearthClient.addDoodad(uri);
         }
      });

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

   },

   _restoreUiState: function() {
      var self = this;

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
            if (!self._state.roofMaterial) {
               self._state.roofMaterial = 0;
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
   _applyRoofGradiantControlState : function(options) {
      var self = this;

      self.$('#roofToolTab .roofDiagramButton').removeClass('active');
      $.each(options.nine_grid_gradiant || [], function(_, dir) {
         self.$('#roofToolTab .roofDiagramButton[gradient="' + dir + '"]').addClass('active');
      });

      $('#roofToolTab #inputMaxRoofHeight').val(options.nine_grid_max_height || 4);
      $('#roofToolTab #inputMaxRoofSlope').val(options.nine_grid_slope || 1);
   },

   _applyControlState: function() {
      var self = this;
      if (self._state) {
         // select default materials
         $(self.$('#floorToolTab .floorMaterial')[self._state.floorMaterial]).addClass('selected');
         $(self.$('#wallToolTab .wallMaterial')[self._state.wallMaterial]).addClass('selected');
         $(self.$('#roofToolTab .roofMaterial')[self._state.roofMaterial]).addClass('selected');
         $(self.$('#doodadToolTab .doodadMaterial')[self._state.doodadMaterial]).addClass('selected');

         // most recently selected tab
         self.$("[tab='" + self._state.activeTabId + "']").click();

         // gradiant on the grow roof control
         self._applyRoofGradiantControlState(self._state.growRoofOptions);
         self._updateGrowRoofOptions()
      }
   },

   _updateSelection: function(building) {
      var self = this;
      var building = this.get(this.uriProperty);
      var building_entity = null;
      var blueprint_entity = null;

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
      }

      self.set('building', building_entity);
      self.set('blueprint', blueprint_entity);

      if (building_entity) {
         self.set('building.active', building_entity['stonehearth:construction_progress'].active);
      }        

      self._updateControls();
   }.observes('context.selection'),

   _updateControls: function() {
      var self = this;

      if(!self.$) {
         return;
      }

      var selectedBuildingWindow = self.$('#selectedBuildingWindow');
      if (!selectedBuildingWindow) {
         return;
      }

      var building_entity = this.get('building');
      var blueprint_entity = this.get('blueprint');

      if (building_entity) {
         selectedBuildingWindow.show();
      } else {
         selectedBuildingWindow.hide();
      }

      if (blueprint_entity) {
         var constructionData = this.get('blueprint.stonehearth:construction_data')
         var type = constructionData.type;

         if (type == 'floor') {
            self.$('.floorToolButton').click();
         } else if (type == 'wall') {           
            self.$('.wallToolButton').click();
         } else if (type == 'roof') {
            self._applyRoofGradiantControlState(constructionData);
            self.$('.roofToolButton').click();
         }
      }
   },
   
});
