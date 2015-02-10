var DrawRoadTool;

(function () {
   DrawRoadTool = SimpleClass.extend({

      toolId: 'drawRoadTool',
      roadMaterialClass: 'roadMaterial',
      curbMaterialClass: 'curbMaterial',
      materialTabId: 'roadMaterialTab',
      roadBrush: null,
      curbBrush: null,

      handlesType: function(type) {
         return type == 'road';
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.buildRoad(self.roadBrush, self.curbBrush == 'none' ? null : self.curbBrush);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Road', self.roadMaterialClass, self.buildingParts.roadPatterns, 
                  function(brush, material) {
                     self.roadBrush = brush;
                     self.roadMaterial = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('roadMaterial', self.roadMaterial);

                     // Re/activate the road tool with the new material.
                     self.buildingDesigner.activateTool(self.buildTool);
                  }
               );
               MaterialHelper.addMaterialPalette(tab, 'Curb', self.curbMaterialClass, self.buildingParts.curbPatterns, 
                  function(brush, material) {
                     self.curbBrush = brush;
                     self.curbMaterial = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('curbMaterial', self.curbMaterial);

                     // Re/activate the road tool with the new material.
                     self.buildingDesigner.activateTool(self.buildTool);
                  }
               );
         });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Draw road'})
         );
      },

      restoreState: function(state) {
         this.roadMaterial = state.roadMaterial || 0;
         var selectedMaterial = $($('#' + this.materialTabId + ' .' + this.roadMaterialClass)[this.roadMaterial]);
         selectedMaterial.addClass('selected');
         this.roadBrush = selectedMaterial.attr('brush');

         this.curbMaterial = state.curbMaterial || 0;
         selectedMaterial = $($('#' + this.materialTabId + ' .' + this.curbMaterialClass)[this.curbMaterial]);
         selectedMaterial.addClass('selected');
         this.curbBrush = selectedMaterial.attr('brush');
      }
   });
})();
