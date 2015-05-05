var DrawVoxelTool;

(function () {
   DrawVoxelTool = SimpleClass.extend({

      toolId:     undefined,
      mode:       undefined,

      init: function(o) {
         this.toolId = o.toolId;
         this.mode = o.mode;
         this.materialClass = o.toolId + 'Material';
      },

      handlesType: function(type) {
         return false;
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               if (self.mode == 'erase') {
                  return App.stonehearthClient.eraseVoxels();                  
               }
               var brush = self._materialHelper.getSelectedBrush();
               return App.stonehearthClient.drawVoxels(brush, self.mode);
            });
      },

      addTabMarkup: function(root) {
         var name;
         if (this.mode == 'erase') {
            return;
         } else if (this.mode == 'paint') {
            name = 'Replace Voxels';
         } else {
            name = 'Add Voxels'
         }

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
                                                   name,
                                                   self.materialClass,
                                                   brushes.voxel,
                                                   brushes.pattern,
                                                   true,
                                                   click);
      },

      restoreState: function(state) {
         if (this._materialHelper) {
            this._materialHelper.restoreState(state);
         }
      }
   });
})();
