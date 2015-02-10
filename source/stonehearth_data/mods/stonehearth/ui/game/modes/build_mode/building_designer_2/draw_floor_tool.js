var DrawFloorTool;

(function () {
   DrawFloorTool = SimpleClass.extend({

      toolId: 'drawFloorTool',
      materialClass: 'floorMaterial',
      materialTabId: 'floorMaterialTab',
      brush: null,

      handlesType: function(type) {
         return type == 'floor';
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.buildFloor(self.brush);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Floor', self.materialClass, self.buildingParts.floorPatterns, 
                  function(brush) {
                     self.brush = brush;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('floorBrush', brush);

                     // Re/activate the floor tool with the new material.
                     self.buildingDesigner.activateTool(self.buildTool);
                  }
               );
         });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Draw floor'})
         );
      },

      restoreState: function(state) {
         var self = this;

         var selector = state.floorBrush ? '[brush="' + state.floorBrush + '"]' :  '.' + self.materialClass;
         var selectedMaterial = $($(selector)[0]);
         
         selectedMaterial.addClass('selected');
         self.brush = selectedMaterial.attr('brush');
      }
   });
})();
