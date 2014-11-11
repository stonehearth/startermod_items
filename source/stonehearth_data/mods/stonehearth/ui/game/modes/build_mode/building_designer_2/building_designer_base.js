App.StonehearthBuildingDesignerBaseTools = App.View.extend({
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

   tools: {},
   actions: [],

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

   saveKey: function(key, value) {
      this._state[key] = value;
      radiant.call('stonehearth:save_browser_object', 'stonehearth:building_designer', this._state);
   },

   newTool: function(ctor) {
      var t = new ctor();
      this.tools[t.toolId] = t;
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      // undo/redoo tool
      this.$('#undoTool').click(function() {
         if (self.get('building')) {
            App.stonehearthClient.undo();
         }
      });

      var doEraseStructure = function() {
         App.stonehearthClient.eraseStructure(
            self.activateElement('#eraseStructureTool'))
            .fail(self._deactivateTool('#eraseStructureTool'))
            .done(function() {
               doEraseStructure();
            });
      };

      this.$('#eraseStructureTool').click(function() {
         doEraseStructure();
      });

      self._state = {};

      $.each(this.tools, function(_, tool) {
         tool.addTabMarkup(self.$('#tabs'));
         tool.addButtonMarkup(self.$('#tools'));
         tool.inDom(self);
      });

      self._addEventHandlers();
      self._restoreUiState();
      self._updateControls();

      self.$("[title]").tooltipster();
   },

   getBlueprint: function() {
      return this.get('blueprint');
   },

   getConstructionData: function() {
      return this.get('blueprint.stonehearth:construction_data');
   },

   activateElement: function(elPath) {
      var self = this;
      return function() {
         if (self.$(elPath)) {
            self.$(elPath).addClass('active');   
         }
      }
   },

   // Called by a building tool; used to hook up to the building designer's flow.
   addTool: function(toolId) {
      var self = this;
      var buildTool = {};

      buildTool.repeat = true;
      buildTool.toolId = toolId;
      buildTool.precall = function() {
         self.activateElement('#' + toolId);
         return buildTool;
      };
      buildTool.invoke = function(invoke_fn) {
         this.invokeTool = invoke_fn;
         return buildTool;
      };

      this.actions[toolId] = buildTool;

      return buildTool;
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
         toolId = tool.attr('id');
         if(self.tools[toolId]) {
            self.tools[toolId].restoreState(self._state)
            if (self.actions[toolId]) {
               App.stonehearthClient.callTool(self.actions[toolId]);
            }
         }
      });

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

            $.each(self.tools, function(_, tool) {
               tool.restoreState(self._state);
            });
         });
   },

   _updateSelection: function() {
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

         $.each(self.tools, function(_, tool) {
            if (tool.handlesType && tool.handlesType(type)) {
               self.$('.tabPage').hide();
               self.$('#' + tool.materialTabId).show();
               self.showEditor();
            }
         });
      } else {
         self.showEditor();
      }
   },
});

