var GrowRoofTool;

(function () {
   GrowRoofTool = SimpleClass.extend({

      toolId: 'growRoofTool',
      materialClass: 'roofMaterial',
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
               self.options.brush = self._materialHelper.getSelectedBrush();
               return App.stonehearthClient.growRoof(self.options);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         var brushes = self.buildingDesigner.getBuildBrushes();

         var click = function(brush) {            
            var blueprint = self.buildingDesigner.getBlueprint();
            self.options.brush = brush
            App.stonehearthClient.setGrowRoofOptions(self.options);         
            if (blueprint) {
               App.stonehearthClient.applyConstructionDataOptions(blueprint, self.options);
            } else {
               // Re/activate the floor tool with the new material.
               self.buildingDesigner.activateTool(self.buildTool);               
            }            
      };

         var tab = $('<div>', { id:self.toolId, class: 'tabPage'} );
         root.append(tab);

         self._materialHelper = new MaterialHelper(tab,
                                                   self.buildingDesigner,
                                                   'Roof',
                                                   self.materialClass,
                                                   brushes.roof,
                                                   null,
                                                   false,
                                                   click);

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
      },

      copyMaterials: function(blueprint) {
         var self = this;
         var roof = blueprint['stonehearth:roof'];
         if (roof) {
            if (roof.brush) {
               self._materialHelper.selectBrush(roof.brush);
            }
            if (roof.nine_grid_gradiant) {
               this.options.nine_grid_gradiant = roof.nine_grid_gradiant;
            }
            if (roof.nine_grid_max_height) {
               this.options.nine_grid_max_height = roof.nine_grid_max_height;
            }
            if (roof.nine_grid_slope) {
               this.options.nine_grid_slope = roof.nine_grid_slope;
            }
            self._updateControlState();
            App.stonehearthClient.setGrowRoofOptions(self.options);
            return;
         }
      },

      _updateControlState: function() {
         $('.roofDiagramButton').removeClass('active');
         $.each(this.options.nine_grid_gradiant || [], function(_, dir) {
            $('.roofDiagramButton[gradient="' + dir + '"]').addClass('active');
         });

         $('#inputMaxRoofHeight').val(this.options.nine_grid_max_height || 4);
         $('#inputMaxRoofSlope').val(this.options.nine_grid_slope || 1);
      },

      restoreState: function(state) {
         var self = this;

         this._materialHelper.restoreState(state);

         this.options = state.growRoofOptions || {
            nine_grid_gradiant: ['left', 'right'],
            nine_grid_max_height: 4,
            nine_grid_slope: 1
         };
         self._updateControlState();
      }
   });
})();
