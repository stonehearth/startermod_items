var GrowRoofTool;

(function () {
   GrowRoofTool = SimpleClass.extend({

      toolId: 'growRoofTool',
      materialClass: 'roofMaterial',
      materialTabId: 'roofMaterialTab',
      material: null,
      brush: null,
      options: {},

      handlesType: function(type) {
         return type == 'roof';
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.growRoof(self.brush);
            });
      },

      // xxx: this helper function is copied verbatim from the grow_walls tool, which is copied
      // verbatim from the draw walls tool.  Maybe we should factor it up to a common place 
      // somewhere?
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
               MaterialHelper.addMaterialPalette(tab, 'Roof', self.materialClass, self.buildingParts.roofPatterns, 
                  function(brush, material) {
                     self.brush = brush;
                     self.material = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('roofMaterial', self.material);
                     self.buildingDesigner.reactivateTool(self.buildTool);

                     self._onMaterialChange('roof', self.brush);
                  }
               );

               $.get('/stonehearth/ui/game/modes/build_mode/building_designer_2/roof_shape_template.html')
                  .done(function(html) {

                     // Seriously.  Thanks, Ember!
                     var o = {data:{buffer:[]}};
                     var h = Ember.Handlebars.compile(html)(null, o);
                     tab.append(o.data.buffer.join(''));

                     tab.find('.roofNumericInput').change(function() {
                        // update the options for future roofs
                        var numericInput = $(this);
                        if (numericInput.attr('id') == 'inputMaxRoofHeight') {
                           self.options.nine_grid_max_height = parseInt(numericInput.val());
                        } else if (numericInput.attr('id') == 'inputMaxRoofSlope') {
                           self.options.nine_grid_slope = parseInt(numericInput.val());
                        }
                        self.buildingDesigner.saveKey('growRoofOptions', self.options);

                        App.stonehearthClient.setGrowRoofOptions(self.options);

                        // if a roof is selected, change it too
                        var blueprint = self.buildingDesigner.getBlueprint();
                        if (blueprint) {
                           App.stonehearthClient.applyConstructionDataOptions(blueprint, self.options);
                        }
                     });

                     // roof slope buttons
                     tab.find('.roofDiagramButton').click(function() {
                        // update the options for future roofs
                        $(this).toggleClass('active');

                        self.options.nine_grid_gradiant = [];
                        tab.find('.roofDiagramButton.active').each(function() {
                           if($(this).is(':visible')) {
                              self.options.nine_grid_gradiant.push($(this).attr('gradient'));
                           }
                        });
                        App.stonehearthClient.setGrowRoofOptions(self.options);
                        
                        self.buildingDesigner.saveKey('growRoofOptions', self.options);

                        // if a roof is selected, change it too
                        var blueprint = self.buildingDesigner.getBlueprint();
                        if (blueprint) {
                           App.stonehearthClient.applyConstructionDataOptions(blueprint, self.options);
                        }
                     });                  
                  });
            });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Grow roof on walls'})
         );
      },

      restoreState: function(state) {
         var self = this;

         $('#' + this.materialTabId + ' .' + this.materialClass).removeClass('selected');
         this.material = state.roofMaterial || 0;
         var selectedMaterial = $($('#' + this.materialTabId + ' .' + this.materialClass)[this.material]);
         selectedMaterial.addClass('selected');
         this.brush = selectedMaterial.attr('brush');

         this.options = state.growRoofOptions || {
            nine_grid_gradiant: ['left', 'right'],
            nine_grid_max_height: 4,
            nine_grid_slope: 1
         };
         $('.roofDiagramButton').removeClass('active');
         $.each(this.options.nine_grid_gradiant || [], function(_, dir) {
            $('.roofDiagramButton[gradient="' + dir + '"]').addClass('active');
         });

         $('#inputMaxRoofHeight').val(this.options.nine_grid_max_height || 4);
         $('#inputMaxRoofSlope').val(this.options.nine_grid_slope || 1);

         App.stonehearthClient.setGrowRoofOptions(self.options);
      }
   });
})();
