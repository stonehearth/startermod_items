var DrawWallTool;

(function () {
   DrawWallTool = SimpleClass.extend({

      toolId: 'drawWallTool',
      wallMaterialClass: 'wallMaterial',
      columnMaterialClass: 'columnMaterial',

      handlesType: function(type) {
         return type == 'wall' || type == 'column';
      },


      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               var wall = self._wallMaterial.getSelectedBrush();
               var column = self._columnMaterial.getSelectedBrush();
               return App.stonehearthClient.buildWall(column, wall);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         var brushes = self.buildingDesigner.getBuildBrushes();

         var wallClick = function(brush) {
            // Re/activate the floor tool with the new material.
            var blueprint = self.buildingDesigner.getBlueprint();
            if (blueprint) {
              radiant.call_obj('stonehearth.build', 'repaint_wall_command', blueprint.__self, brush);
            } else {
              self.buildingDesigner.activateTool(self.buildTool);
            }
         };

         var columnClick = function(brush) {
            // Re/activate the floor tool with the new material.
            var blueprint = self.buildingDesigner.getBlueprint();
            if (blueprint) {
              radiant.call_obj('stonehearth.build', 'repaint_column_command', blueprint.__self, brush);
            } else {
              self.buildingDesigner.activateTool(self.buildTool);
            }
         };

         var tab = $('<div>', { id:self.toolId, class: 'tabPage'} );
         root.append(tab);

         self._wallMaterial = new MaterialHelper(tab,
                                                 self.buildingDesigner,
                                                 'Wall',
                                                 self.wallMaterialClass,
                                                 brushes.wall,
                                                 null,
                                                 false,
                                                 wallClick);

         self._columnMaterial = new MaterialHelper(tab,
                                                 self.buildingDesigner,
                                                 'Column',
                                                 self.columnMaterialClass,
                                                 brushes.column,
                                                 null,
                                                 false,
                                                 columnClick);
      },

      copyMaterials: function(blueprint) {
         var self = this;
         var wall = blueprint['stonehearth:wall'];
         if (wall) {
            self._wallMaterial.selectBrush(wall.brush);
            if (wall.column_a) {
               self._columnMaterial.selectBrush(wall.column_a['stonehearth:column'].brush);
            }
            return;
         }
         var column = blueprint['stonehearth:column'];
         if (column) {
            self._columnMaterial.selectBrush(column.brush);
         }
      },

      restoreState: function(state) {
         this._wallMaterial.restoreState(state);
         this._columnMaterial.restoreState(state);
      }
   });
})();
