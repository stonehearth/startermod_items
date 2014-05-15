App.StonehearthBuildingDesignerView = App.View.extend({
	templateName: 'buildingDesigner',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen'],


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
   }.observes('context'),

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

      this.$('.floorTool').click(function() {
         App.stonehearthClient.buildFloor();
      });
      this.$('.fillWallTool').click(function() {
         App.stonehearthClient.growWalls();
      });
      this.$('.roofTool').click(function() {
         App.stonehearthClient.growRoof();
      });
      this.$('#startBuilding').click(function() {
         var building_entity = self.get('context.building');
         if (building_entity) {
            var value = !self.get('context.building.active')
            //stonehearth.server.build.set_building_active(building_entity.__self, value);
            radiant.call('stonehearth:set_building_active', building_entity.__self, value)
         }
      });
      this.$('#removeBuilding').click(function() {
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
