App.StonehearthBuildingDesignerTools = App.View.extend({
   templateName: 'buildingDesigner',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', "gui"],
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

   showOverview: function() {
      this.$('#editor').hide();
      this.$('#overview').show();
   },

   showEditor: function() {
      this.$('#editor').show();
      this.$('#overview').hide();

      // activate the first tool
      var toolButtons = this.$('.toolButton');
      if (toolButtons[0]) {
         //toolButtons[0].click();
      }
   },

   // Save the state of the dialog int the 'stonehearth:building_designer' key.
   _saveState: function() {
      radiant.call('stonehearth:save_browser_object', 'stonehearth:building_designer', this._state);
   },

   _buildMaterialPalette: function(materials, materialClassName) {
      var palette = $('<div>').addClass('brushPalette');

      // for each category
      var index = 0;
      $.each(materials, function(i, category) {
         // for each material
         $.each(category.items, function(k, material) {
            var brush = $('<div>')
                           .addClass('brush')
                           .attr('brush', material.brush)
                           .css({ 'background-image' : 'url(' + material.portrait + ')' })
                           .attr('title', material.name)
                           .attr('index', index)
                           .addClass(materialClassName)
                           .append('<div class=selectBox />'); // for showing the brush when it's selected

            palette.append(brush);
            

            index += 1;
         });
      });

      return palette;
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
            self.$('#floorMaterials').append(self._buildMaterialPalette(self.buildingParts.floorPatterns, 'floorMaterial'));
            self.$('#wallMaterials').append(self._buildMaterialPalette(self.buildingParts.wallPatterns, 'wallMaterial'));
            self.$('#roofMaterials').append(self._buildMaterialPalette(self.buildingParts.roofPatterns, 'roofMaterial'));
            self.$('#slabMaterials').append(self._buildMaterialPalette(self.buildingParts.slabPatterns, 'slabMaterial'));
            self.$('#doodadMaterials').append(self._buildMaterialPalette(self.buildingParts.doodads, 'doodadMaterial'));

            self._addEventHandlers();
            self._restoreUiState();
            self._updateControls();

            self.$("[title]").tooltipster();
         });
   },

   _deactivateTool: function(toolTag) {
      var self = this;
      return function() {
         if (self.$(toolTag)) {
            self.$(toolTag).removeClass('active');
         }
         $(top).trigger('radiant_hide_tip');
      }
   },

   _addEventHandlers: function() {
      var self = this;

      // tab buttons and pages
      this.$('.toolButton').click(function() {
         var tool = $(this);
         var tabId = tool.attr('tab');
         var tab = self.$('#' + tabId);

         if (!tabId) {
            return;
         }

         // show the correct tab page
         self.$('.tabPage').hide();
         tab.show();

         // activate the tool
         self.$('.toolButton').removeClass('active');
         tool.addClass('active');
         
         // update the material in the tab to reflect the selection
         self._selectActiveMaterial(tab);

      });

      // undo/redoo tool
      this.$('#undoTool').click(function() {
         if (self.get('building')) {
            App.stonehearthClient.undo();
         }
      });

      var activateElement = function(elPath) {
         return function() {
            if (self.$(elPath)) {
               self.$(elPath).addClass('active');   
            }
         }
      };

      // draw floor tool
      var doDrawFloor = function() {
         var brush = self.$('#floorMaterialTab .floorMaterial.selected').attr('brush');
         App.stonehearthClient.buildFloor(brush, 
            activateElement('#drawFloorTool'))
            .fail(self._deactivateTool('#drawFloorTool'))
            .done(function() {
               doDrawFloor();
            });
      };

      var doEraseStructure = function() {
         App.stonehearthClient.eraseStructure(
            activateElement('#eraseStructureTool'))
            .fail(self._deactivateTool('#eraseStructureTool'))
            .done(function() {
               doEraseStructure();
            });
      };

      // slab tool
      var doDrawSlab = function() {
         var brush = self.$('#slabMaterialTab .slabMaterial.selected').attr('brush');
         App.stonehearthClient.buildSlab(brush, 
            activateElement('#drawSlabfTool'))
            .fail(self._deactivateTool('#drawSlabfTool'))
            .done(function() {
               doDrawSlab();
            });
      };

      // wall tool tab

      // draw wall tool
      var doDrawWall = function() {
         var wallUri = self.$('#wallMaterialTab .wallMaterial.selected').attr('brush');
         App.stonehearthClient.buildWall('stonehearth:wooden_column', wallUri, 
            activateElement('#drawWallTool'))
            .fail(self._deactivateTool('#drawWallTool'))
            .done(function() {
               doDrawWall(false);
            });
      };


      var doGrowWalls = function() {
         var wallUri = self.$('#wallMaterialTab .wallMaterial.selected').attr('brush');
         App.stonehearthClient.growWalls('stonehearth:wooden_column', wallUri,
            activateElement('#growWallsTool'))
            .fail(self._deactivateTool('#growWallsTool'))
            .done(function() {
               doGrowWalls();
            });
      };


      // roof tab
      var doGrowRoof = function() {
         var roofUri = self.$('#roofMaterialTab .roofMaterial.selected').attr('brush');
         App.stonehearthClient.growRoof(roofUri,
            activateElement('#growRoofTool'))
            .fail(self._deactivateTool('#growRoofTool'))
            .done(function() {
               doGrowRoof();
            });
      };

      // draw doodad tool
      var doAddDoodad = function() {
         var uri = self.$('#doodadMaterialTab .doodadMaterial.selected').attr('brush');
         App.stonehearthClient.addDoodad(uri,
            activateElement('#drawDoodadTool'))
            .fail(self._deactivateTool('#drawDoodadTool'))
            .done(function() {
               doAddDoodad();
            });
      }

      this.$('#drawFloorTool').click(function() {
         doDrawFloor();
      });

      this.$('#eraseStructureTool').click(function() {
         doEraseStructure();
      });

      this.$('#drawWallTool').click(function() {
         doDrawWall(true);
      });

      // grow walls tool
      this.$('#growWallsTool').click(function() {
         doGrowWalls();
      });

      this.$('#growRoofTool').click(function() {
         doGrowRoof();
      });

      this.$('#drawDoodadTool').click(function() {
         doAddDoodad();
      });

      // floor materials
      this.$('.floorMaterial').click(function() {
         self.$('.floorMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.floorMaterial = $(this).attr('index');
         self._saveState();

         // Re/activate the floor tool with the new material
         doDrawFloor(true);
      })      

      // wall materials
      this.$('#wallMaterialTab .wallMaterial').click(function() {
         // select the clicked material
         self.$('#wallMaterialTab .wallMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.wallMaterial = $(this).attr('index');
         self._saveState();

         // update the selected building part, if there is one
         var blueprint = self.get('blueprint');
         var constructionData = self.get('blueprint.stonehearth:construction_data');

         if (blueprint && constructionData && constructionData.type == 'wall') {
            var wallUri = $(this).attr('brush');
            App.stonehearthClient.replaceStructure(blueprint, wallUri);
         }
         // Reactivate the active tool with the new material, if an active
         // tool exists.  Otherwise, just select the 'draw wall' tool.
         if (self.$('#growWallsTool').hasClass('active')) {
            doGrowWalls();
         } else if (self.$('#drawWallTool').hasClass('active')) {
            doDrawWall(true);               
         }
      });

      // slab materials
      this.$('.slabMaterial').click(function() {
         self.$('.slabMaterial').removeClass('selected');
         $(this).addClass('selected');

         self._state.slabMaterial = $(this).attr('index');
         self._saveState();

         // Re/activate the floor tool with the new material
         doDrawSlab(true);
      })      


      this.$('#roofMaterialTab .roofMaterial').click(function() {
         // select the clicked material
         self.$('#roofMaterialTab .roofMaterial').removeClass('selected');
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

         // Re/activate the active tool with the new material. 
         doGrowRoof();
      });

      this.$('#roofMaterialTab .roofNumericInput').change(function() {
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
      this.$('#roofMaterialTab .roofDiagramButton').click(function() {
         // update the options for future roofs
         $(this).toggleClass('active');
         self._getRoofOptionsFromElement('#roofMaterialTab', self._state.growRoofOptions);
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

         // reactivate the doodad tool with the current material
         doAddDoodad();
      })

      // building buttons
      this.$('#showOverview').click(function() {
         self.showOverview();
      });

      this.$('#showEditor').click(function() {
         self.showEditor();
      });


      this.$('#startBuilding').click(function() {
         var doStartBuilding = function() {
            App.stonehearthClient.deactivateAllTools();
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:submenu_select'} );
            var building_entity = self.get('building');
            if (building_entity) {
               var value = !self.get('building.active')
               radiant.call('stonehearth:set_building_active', building_entity.__self, value);

               //xxx hack! The server should do this for us
               self.set('building.active', true);
            }
         }

         App.gameView.addView(App.StonehearthConfirmView, 
            { 
               title : "Really start building?",
               message : "Once you begin building this structure it cannot be edited. Are you sure you want to build now?",
               buttons : [
                  { 
                     id: 'confirmBuilding',
                     label: "Yes, start building!",
                     click: doStartBuilding
                  },
                  {
                     label: "Not yet, I have more edits"
                  }
               ] 
            });         
      });

      this.$('.removeBuilding').click(function() {
         var doRemoveBuilding = function() {
            var building_entity = self.get('building');
            if (building_entity) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:trash'} );
               radiant.call('stonehearth:set_building_teardown', building_entity.__self, true)
               self.set('context.selection', null);
               $(top).trigger('stonehearth_building_templates');
            }
         }

         App.gameView.addView(App.StonehearthConfirmView, 
            { 
               title : "Really remove this building",
               message : "Are you sure you want to remove this entire building?",
               buttons : [
                  { 
                     id: 'confirmRemove',
                     label: "Keep this building"
                  },
                  {
                     label: "Remove this building",
                     click: doRemoveBuilding
                  }
               ] 
            });         
      });

      this.$('#saveTemplate').click(function() {
         var building = self.get('building');

         if (building) {
            App.gameView.addView(App.StonehearthTemplateNameView, { 
               building : building
            });
         }
      });
   },

   _restoreUiState: function() {
      var self = this;

      // restore the state of the dialog from the last time it was invoked and 
      radiant.call('stonehearth:load_browser_object', 'stonehearth:building_designer')
         .done(function(o) {
            self._state = o.value || {};
            if (!self._state.floorMaterial) {
               self._state.floorMaterial = 0;
            }
            if (!self._state.wallMaterial) {
               self._state.wallMaterial = 0;
            }
            if (!self._state.roofMaterial) {
               self._state.roofMaterial = 0;
            }
            if (!self._state.slabMaterial) {
               self._state.slabMaterial = 0;
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

      self.$('#roofMaterialTab .roofDiagramButton').removeClass('active');
      $.each(options.nine_grid_gradiant || [], function(_, dir) {
         self.$('#roofMaterialTab .roofDiagramButton[gradient="' + dir + '"]').addClass('active');
      });

      $('#roofMaterialTab #inputMaxRoofHeight').val(options.nine_grid_max_height || 4);
      $('#roofMaterialTab #inputMaxRoofSlope').val(options.nine_grid_slope || 1);
   },

   _applyControlState: function() {
      var self = this;
      if (self._state) {
         // select default materials
         $(self.$('#floorMaterialTab .floorMaterial')[self._state.floorMaterial]).addClass('selected');
         $(self.$('#wallMaterialTab .wallMaterial')[self._state.wallMaterial]).addClass('selected');
         $(self.$('#roofMaterialTab .roofMaterial')[self._state.roofMaterial]).addClass('selected');
         $(self.$('#slabMaterialTab .slabMaterial')[self._state.slabMaterial]).addClass('selected');
         $(self.$('#doodadMaterialTab .doodadMaterial')[self._state.doodadMaterial]).addClass('selected');

         // gradiant on the grow roof control
         self._applyRoofGradiantControlState(self._state.growRoofOptions);
         self._updateGrowRoofOptions()

         self.$('.tabPage').hide();
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

         self.$('.bottomButtons').show();
      } else {
         self.$('.bottomButtons').hide();
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

      var bottomButtons = self.$('.bottomButtons');
      if (!bottomButtons) {
         return;
      }

      var building_entity = this.get('building');
      var blueprint_entity = this.get('blueprint');

      if (building_entity) {
         // refresh the building cost
         App.stonehearthClient.getCost(building_entity.__self)
            .done(function(response) {
               var costView = self._getClosestEmberView(self.$('#buildingCost'));
               costView.set('cost', response);
            })
            .fail(function(response) {
               console.log(response);
            });
      
         bottomButtons.show();
      } else {
         bottomButtons.hide();
      }

      if (blueprint_entity) {
         var constructionData = this.get('blueprint.stonehearth:construction_data')
         var type = constructionData.type;


         if (type == 'floor') {
            self.$('.tabPage').hide();
            self.$('#floorMaterialTab').show();
         } else if (type == 'slab') {           
            self.$('.tabPage').hide();
            self.$('#slabMaterialTab').show();
         } else if (type == 'wall') {           
            self.$('.tabPage').hide();
            self.$('#wallMaterialTab').show();
         } else if (type == 'roof') {
            self._applyRoofGradiantControlState(constructionData);
            self.$('.tabPage').hide();
            self.$('#roofMaterialTab').show();
         }
      }
   },

});

App.StonehearthTemplateNameView = App.View.extend({
   templateName: 'templateNameView',
   classNames: ['fullScreen', "flex"],

   didInsertElement: function() {
      var self = this;
      this._super();

      this.$('#name').val('binky');
      this.$('#name').focus();

      this.$('#name').keypress(function (e) {
         if (e.which == 13) {
            $(this).blur();
            self.$('.ok').click();
        }
      });

      this.$('.ok').click(function() {
         var templateDisplayName = self.$('#name').val()
         var templateName = templateDisplayName.split(' ').join('_').toLowerCase();

         radiant.call('stonehearth:save_building_template', self.building.__self, { 
            name: templateName,
            display_name: templateDisplayName,
         })

         self.destroy();

      });
   },

});
