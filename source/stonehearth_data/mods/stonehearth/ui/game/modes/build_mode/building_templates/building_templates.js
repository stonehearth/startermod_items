App.StonehearthBuildingTemplatesView = App.View.extend({
   templateName: 'buildingTemplates',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', "gui"],

   components: {
   },

   init: function() {
      var self = this;

      this._super();
   },   

   didInsertElement: function() {
      this._super();
      var self = this;
      this._addEventHandlers();
   },

   show: function() {
      this._loadTemplates();
      this._super();
   },

   _loadTemplates: function() {
      var self = this;
      radiant.call('stonehearth:get_building_templates')
         .done(function(response) {
            self._templatesMap = response
            self.set('context.templates', self._mapToArrayObject(response));
         });

   },

   _addEventHandlers: function() {
      var self = this

      self.$().on('click', '#templatesList .row', function() {        
         self.$('#templatesList .row').removeClass('selected');
         var row = $(this);

         row.addClass('selected');
         self._selectedTemplate = row.attr('template');
         App.stonehearthClient.drawTemplate(null, self._selectedTemplate);
      });


      self.$().on('mouseenter', '#templatesList .row', function() {
            var templateName = $(this).attr('template');
            var template = self._templatesMap[templateName];
            self.set('context.selected_template', template);

            var costView = self._getClosestEmberView(self.$('#buildingCost'));
            costView.set('cost', template.cost);

            self.$('#buildingTemplatesDetailsPopup').show()
               .position({
                  my: "left center",
                  at: "right+40 center",
                  of: $(this),
                  collision: 'fit',
               });
         });

      self.$().on('mouseleave', '#templatesList .row', function() {        
            self.$('#buildingTemplatesDetailsPopup').hide();
         });
   
      self.$('#customBuildingButton').click(function() {
         $(top).trigger('stonehearth_building_editor');
      })      
   },

});
