App.StonehearthBuildingDesignerView = App.View.extend({
	templateName: 'buildingDesigner',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen'],


   components: {
      'unit_info': {},
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
            'unit_info' : {}
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
      var construnction_progress_component = context['stonehearth:construnction_progress_component'];
      
      if (fabricator_component) {
         blueprint_entity = fabricator_component['blueprint'];
      } else if (construction_data_component) {
         blueprint_entity = construction_data_component.fabricator_entity['stonehearth:fabricator'].blueprint;
      } else if (construnction_progress_component) {
         blueprint_entity = context
      }
      if (blueprint_entity) {
         building_entity = blueprint_entity['stonehearth:construction_progress']['building_entity'];         
      }
      self.set('context.building', building_entity);
      self.set('context.blueprint', blueprint_entity);
   }.observes('context'),

   didInsertElement: function() {
      var self = this;
      this._super();

      var set_building_active = function(value) {
         var building_entity = self.get('context.building');
         if (building_entity) {
            radiant.call('stonehearth:set_building_active', building_entity.__self, value)
         }
      }

      this.$('#startBuilding').click(function() {
         set_building_active(true);
      });
      this.$('#stopBuilding').click(function() {
         set_building_active(false);
      });
   },

   _onEntitySelected: function(e) {
      var entity = e.selected_entity
      
      if (!entity) {
         return;
      }

      // nuke the old trace
      if (self.selectedEntityTrace) {
         self.selectedEntityTrace.destroy();
      }

      // trace the properties so we can tell if we need to popup the properties window for the object
      self.selectedEntityTrace = radiant.trace(entity)
         .progress(function(result) {
            self._examineEntity(result);
         })
         .fail(function(e) {
            console.log(e);
         });
   },

});
