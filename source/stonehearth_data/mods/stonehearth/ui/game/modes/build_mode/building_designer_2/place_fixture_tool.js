var PlaceFixtureTool;

(function () {
   PlaceFixtureTool = SimpleClass.extend({

      toolId: undefined,
      category: undefined,
      materialClass: undefined,

      init: function(o) {
         this.category = o.category;
         this.toolId = o.toolId;
         this.materialClass = o.materialClass;
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               var brush = self._materialHelper.getSelectedBrush();
               return App.stonehearthClient.addDoodad(brush);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         var brushes = self.buildingDesigner.getBuildBrushes();

         var click = function(brush) {
            // Re/activate the floor tool with the new material.
            self.buildingDesigner.activateTool(self.buildTool);
         };

         var tab = $('<div>', { id:self.toolId, class: 'tabPage'} );
         root.append(tab);

         self._materialHelper = new MaterialHelper(tab,
                                                   self.buildingDesigner,
                                                   self.category,
                                                   self.materialClass,
                                                   brushes[self.category],
                                                   null,
                                                   click);
      },

      restoreState: function(state) {
         this._materialHelper.restoreState(state);
      }
   });
})();
