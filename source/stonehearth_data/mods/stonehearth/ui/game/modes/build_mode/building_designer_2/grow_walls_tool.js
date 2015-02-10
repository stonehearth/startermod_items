var GrowWallsTool;

(function () {
   GrowWallsTool = SimpleClass.extend({

      toolId: 'growWallsTool',
      wallMaterialClass: 'growWallMaterial',
      columnMaterialClass: 'growColumnMaterial',
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
            });
      },

      _onMaterialChange: function(type, brush) {
         var blueprint = this.buildingDesigner.getBlueprint();
         var constructionData = this.buildingDesigner.getConstructionData();

         if (blueprint && constructionData && constructionData.type == type) {
            App.stonehearthClient.replaceStructure(blueprint, brush);
         }
         // Re/activate the tool with the new material.
         this.buildingDesigner.reactivateTool(this.buildTool);
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Wall', self.wallMaterialClass, self.buildingParts.wallPatterns, 
                  function(brush) {
                     self.wallBrush = brush;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('wallBrush', brush);

                     self._onMaterialChange('wall', self.wallBrush);
                  }
               );
               MaterialHelper.addMaterialPalette(tab, 'Column', self.columnMaterialClass, self.buildingParts.columnPatterns, 
                  function(brush, material) {
                     self.columnBrush = brush;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('columnBrush', brush);

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
         var self = this;
         var selector = state.wallBrush ? '.' + self.wallMaterialClass + '[brush="' + state.wallBrush + '"]' :  '.' + self.wallMaterialClass;
         var selectedMaterial = $($(selector)[0]);
         $('.' + self.wallMaterialClass).removeClass('selected');
         selectedMaterial.addClass('selected');
         self.wallBrush = selectedMaterial.attr('brush');

         var selector = state.columnBrush ? '.' + self.columnMaterialClass + '[brush="' + state.columnBrush + '"]' :  '.' + self.columnMaterialClass;
         var selectedMaterial = $($(selector)[0]);
         $('.' + self.columnMaterialClass).removeClass('selected');
         selectedMaterial.addClass('selected');
         self.columnBrush = selectedMaterial.attr('brush');
      }
   });
})();
