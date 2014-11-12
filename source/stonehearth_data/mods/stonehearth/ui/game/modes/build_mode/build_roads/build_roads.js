App.StonehearthBuildRoadsView = App.StonehearthBuildingDesignerBaseTools.extend({
   templateName: 'roadDesigner',

   init: function() {
      var self = this;
      this._super();
   },   

   didInsertElement: function() {
      var self = this;

      this.newTool(DrawRoadTool);

      // Make sure we call super after adding all the tools!
      this._super();
   }
});
