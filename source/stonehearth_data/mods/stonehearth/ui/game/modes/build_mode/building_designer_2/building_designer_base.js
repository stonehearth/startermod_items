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


   init: function() {
      var self = this;

      this.tools = {};
      this.actions = [];
      this._active_tool_stack = [];

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
         tool.addTabMarkup(self.$('#toolOptions'));
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

   _removeTool: function(depth) {
      var toRemove = -1;
      $.each(this._active_tool_stack, function(i, v) {
         if (v[0] == depth) {
            toRemove = i;

            // It's possible we could see more than one of the same stack depth in this
            // array.  I *think* it should be fine, as long as we remove from the
            // bottom to the top, so stop when we see the first matching element.
            return false;
         }
      });
      if (toRemove > -1) {
         this._active_tool_stack.splice(toRemove, 1);
      }      
   },

   _lastTool: function() {
      var l = this._active_tool_stack.length - 1;
      var activeTool = l >= 0 ? this._active_tool_stack[l][1] : null;

      return [l, activeTool];
   },

   _pushTool : function(tool) {
      var stackDepth = this._lastTool()[0] + 1;
      this._active_tool_stack.push([stackDepth, tool]);
   },

   _doToolCall: function(tool) {
      var self = this;

      this._pushTool(tool);
      var stackDepth = this._lastTool()[0];

      App.stonehearthClient.callTool(tool)
         .done(function(response) {
            if (tool.repeat) {
               self.reactivateTool(tool);
               self._removeTool(stackDepth);
            } else {
               self._removeTool(stackDepth);
               if (self._lastTool()[1] == null) {
                  self.$('.toolButton').removeClass('active');
               }
            }
         })
         .fail(function() {
            self._removeTool(stackDepth);
            if (self._lastTool()[1] == null) {
               self.$('.toolButton').removeClass('active');
            }
         });
   },

   activateTool: function(tool) {
      var self = this;
      this.tools[tool.toolId].restoreState(this._state);

      var activeTool = this._lastTool()[1];

      // activate the tool
      this.$('.toolButton').removeClass('active');
      this.$('#' + tool.toolId).addClass('active');

      this._doToolCall(tool);
   },

   reactivateTool: function(tool) {
      var self = this;
      var activeTool = this._lastTool()[1];

      if (activeTool == tool) {
         this._doToolCall(tool);
      }
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
      this.$(".palette area").click(function() {
         var areaTool = $(this).attr('tool');
         var toolId = self.imageMapToTool[areaTool]
         if (!toolId) {
            return;
         }

         var tool = self.tools[toolId];

         var tabId = '#' + tool.materialTabId;
         var tab = self.$(tabId);

         // show the correct tab page
         self.$('.tabPage').hide();
         tab.show();
         
         if (self.actions[toolId]) {
            self.activateTool(self.actions[toolId]);
         }
      });

      // building buttons
      this.$().on( 'click', '#showOverview', function() {
         self.showOverview();
      });


      this.$().on( 'click', '#showEditor', function() {
         self.showEditor();
      });


      this.$().on( 'click', '#startBuilding', function() {
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
               title : i18n.t('start_building_confirm_title'),
               message : i18n.t('start_building_confirm'),
               buttons : [
                  { 
                     id: 'confirmBuilding',
                     label: i18n.t('start_building_yes'),
                     click: doStartBuilding
                  },
                  {
                     label: i18n.t('start_building_no')
                  }
               ] 
            });         
      });

      this.$().on( 'click', '#removeBuilding', function() {
         var doRemoveBuilding = function() {
            var building_entity = self.get('building');
            if (building_entity) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:trash'} );
               radiant.call('stonehearth:set_building_teardown', building_entity.__self, true)
               self.set('context.selection', null);
            }
         }

         App.gameView.addView(App.StonehearthConfirmView, 
            { 
               title : i18n.t('remove_building_title'),
               message : i18n.t('remove_building'), 
               buttons : [
                  { 
                     id: 'confirmRemove',
                     label: i18n.t('remove_building_yes')
                  },
                  {
                     label: i18n.t('remove_building_no'),
                     click: doRemoveBuilding
                  }
               ] 
            });         
      });

      this.$().on( 'click', '#saveTemplate', function() {
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

