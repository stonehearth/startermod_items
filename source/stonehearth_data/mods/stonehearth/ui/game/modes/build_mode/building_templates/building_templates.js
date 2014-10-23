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
      this._updateButtons();
   },

   show: function() {
      this._loadTemplates();
      this._super();
   },

   _loadTemplates: function() {
      var self = this;
      radiant.call('stonehearth:get_building_templates')
         .done(function(response) {
            var templateArray = self._mapToArrayObject(response)
            self.set('context.templates', templateArray);
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
         
         //self._updateButtons();
      });
      
      /*
      this.$('#buildTemplateButton').click(function() {
         App.stonehearthClient.drawTemplate(activateElement('#drawTemplateTool'), self._selectedTemplate);
      });
      */

      self.$('#customBuildingButton').click(function() {
         $(top).trigger('stonehearth_building_editor');
      })      
   },

   _updateButtons: function() {
      var self = this;

      if (self.$('#templatesList .selected').length == 0) {
         self.$('#buildTemplateButton').addClass('disabled');
      } else {
         self.$('#buildTemplateButton').removeClass('disabled');
      }
   }
   
});
