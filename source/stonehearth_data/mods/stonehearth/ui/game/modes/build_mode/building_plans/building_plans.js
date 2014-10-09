App.StonehearthBuildingPlansView = App.View.extend({
   templateName: 'buildingPlans',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', "gui"],

   components: {
   },

   init: function() {
      var self = this;

      this._super();
   },   

   didInsertElement: function() {
      var self = this

      self.$('#customBuildingButton').click(function() {
         $(top).trigger('stonehearth_building_designer');
      })
   }
   
});
