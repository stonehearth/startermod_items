var GrowRoofTool;

(function () {
   GrowRoofTool = SimpleClass.extend({

      toolId: 'growRoofTool',
      materialClass: 'roofMaterial',
      materialTabId: 'roofMaterialTab',
      brush: null,

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         /*this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.growWalls(self.columnBrush, self.wallBrush);
            })
            .done();*/
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
               MaterialHelper.addMaterialPalette(tab, 'Roof Material', self.materialClass, self.buildingParts.roofPatterns, 
                  function(brush, material) {
                     self.brush = brush;
                     self.material = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('roofMaterial', self.material);

                     //self._onMaterialChange('roof', self.brush);
                  }
               );

               // Oh, god, what the hell....
               tab.append($('<div id="roofEditor"><h2>Roof shape thingy</h2><div class="brushPalette downSection"><div class="roofShape"><div id="roofDiagram" /><div class="roofDiagramButton north" gradient="back"/><div class="roofDiagramButton south" gradient="front"/><div class="roofDiagramButton east" gradient="right" /><div class="roofDiagramButton west" gradient="left" /><div id="roofNumerics"><div><div class="label">Max height</div><div class="dec numericButton uisounds">-</div><input type="number" id="inputMaxRoofHeight" value="4" min="1" max="8" class="numericInput roofNumericInput"><div class="inc numericButton uisounds">+</div><div style="clear: both;" /></div></div></div></div></div>'));

               /*tab.append(
                  // Sigh.  Is there a (simple) way to find and render a handlebars template in ember?
                  $('div', {id:'roofEditor'})
                     .append($('h2').text(i18n.t('stonehearth:building_designer_roof_shape')))
                     .append($('div').addClass('brushPalette').addClass('downSection')
                        .append($('div').addClass('roofShape')
                           .append($('div', {id : 'roofDiagram'}))
                           .append($('div', {gradient: 'back'}).addClass('roofDiagramButton').addClass('north'))
                           .append($('div', {gradient: 'front'}).addClass('roofDiagramButton').addClass('south'))
                           .append($('div', {gradient: 'right'}).addClass('roofDiagramButton').addClass('east'))
                           .append($('div', {gradient: 'left'}).addClass('roofDiagramButton').addClass('west'))
                           .append($('div', {id: 'roofNumerics'})
                              .append($('div')
                                 .append($('div'))))))/*.text("Max height"))
                                 .append($('div').addClass('dec').addClass('numericButton').addClass('uisounds').text('-'))
                                 .append($('input', {id: 'inputMaxRoofHeight', type: 'number'}).addClass('numericInput').addClass('roofNumericInput'))
                                 .append($('div').addClass('inc').addClass('numericButton').addClass('uisounds').text('+'))
                                 .append($('div', {style: 'clear:both;'}))
                              )
                           )
                        )
                     )
               );*/
            });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Grow roof on walls'})
         );
      },

      restoreState: function(state) {
         /*$('#' + this.materialTabId + ' .' + this.columnMaterialClass).removeClass('selected');
         this.columnMaterial = state.columnMaterial || 0;
         var selectedMaterial = $($('#' + this.materialTabId + ' .' + this.columnMaterialClass)[this.columnMaterial]);
         selectedMaterial.addClass('selected');
         this.columnBrush = selectedMaterial.attr('brush');

         $('#' + this.materialTabId + ' .' + this.wallMaterialClass).removeClass('selected')
         this.wallMaterial = state.wallMaterial || 0;
         selectedMaterial = $($('#' + this.materialTabId + ' .' + this.wallMaterialClass)[this.wallMaterial]);
         selectedMaterial.addClass('selected');
         this.wallBrush = selectedMaterial.attr('brush');*/
      }
   });
})();
