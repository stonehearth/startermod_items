var GrowWallsTool;

(function () {
   GrowWallsTool = SimpleClass.extend({

      toolId: 'growWallsTool',
      wallMaterialClass: 'wallMaterial',
      columnMaterialClass: 'columnMaterial',
      materialTabId: 'growWallMaterialTab',
      columnBrush: null,
      wallBrush: null,

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.growWalls(self.columnBrush, self.wallBrush);
            })
            .done();
      },

      _onMaterialChange: function(type, brush) {
         var blueprint = this.buildingDesigner.getBlueprint();
         var constructionData = this.buildingDesigner.getConstructionData();

         if (blueprint && constructionData && constructionData.type == type) {
            App.stonehearthClient.replaceStructure(blueprint, brush);
         }
         // Re/activate the tool with the new material.
         App.stonehearthClient.callTool(this.buildTool);
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Wall Material', self.wallMaterialClass, self.buildingParts.wallPatterns, 
                  function(brush, material) {
                     self.wallBrush = brush;
                     self.wallMaterial = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('wallMaterial', self.wallMaterial);

                     self._onMaterialChange('wall', self.wallBrush);
                  }
               );
               MaterialHelper.addMaterialPalette(tab, 'Column Material', self.columnMaterialClass, self.buildingParts.columnPatterns, 
                  function(brush, material) {
                     self.columnBrush = brush;
                     self.columnMaterial = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('columnMaterial', self.columnMaterial);

                     self._onMaterialChange('column', self.columnBrush);
                  }
               );
         });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Grow walls'})
         );
      },

      restoreState: function(state) {
         $('#' + this.materialTabId + ' .' + this.columnMaterialClass).removeClass('selected');
         this.columnMaterial = state.columnMaterial || 0;
         var selectedMaterial = $($('#' + this.materialTabId + ' .' + this.columnMaterialClass)[this.columnMaterial]);
         selectedMaterial.addClass('selected');
         this.columnBrush = selectedMaterial.attr('brush');

         $('#' + this.materialTabId + ' .' + this.wallMaterialClass).removeClass('selected')
         this.wallMaterial = state.wallMaterial || 0;
         selectedMaterial = $($('#' + this.materialTabId + ' .' + this.wallMaterialClass)[this.wallMaterial]);
         selectedMaterial.addClass('selected');
         this.wallBrush = selectedMaterial.attr('brush');
      }
   });
})();
