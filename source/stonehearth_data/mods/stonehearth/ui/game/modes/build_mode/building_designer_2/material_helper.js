var MaterialHelper = SimpleClass.extend({
   init : function(tab, buildingDesigner, tabTitle, materialClass, colors, patterns, clickHandler) {
      var self = this;

      self._tabTitle = tabTitle;
      self._buildingDesigner = buildingDesigner;

      self._container = $('<div>')
      self._materialClass = materialClass;
      self._clickHandler = clickHandler;

      tab.append(self._container)

      self._addMaterialPalette(colors, patterns);
   },

   _buildMaterialPalette : function(palette, category, brushes) {
      // for each category
      var self = this;
      $.each(brushes, function(i, material) {
         var brush = $('<div>')
                        .addClass('brush')
                        .addClass(self._materialClass)
                        .attr('brush', material)
                        .attr('category', category)
                        .append('<div class=selectBox />'); // for showing the brush when it's selected
         if (material[0] == '#') {
            brush.css({ 'background-color' : material });
         } else {
            brush.addClass(material.split(':').pop());
         }
         palette.append(brush);
      });
   },

   _selectBrush: function(brush, skipSave) {
      var container = this._container;
      var selectedMaterial;

      if (brush) {
         selectedMaterial = $(container.find('.brush[brush="' + brush + '"]')[0]);
         // if the brush doesn't exist, someone changed the backing
         // store.  pick the default
         if (!selectedMaterial[0]) {
            brush = undefined;
         }
      }
      if (!brush) {
         selectedMaterial = $(container.find('.brush')[0]);
         brush = selectedMaterial.attr('brush');
      }
      var category = selectedMaterial.attr('category');

      this._state.brush = brush
      this._state.brushes[category] = brush
      if (!skipSave) {
         this._buildingDesigner.saveKey(this._materialClass, this._state);
      }

      container.find('.button').removeClass('selected');
      container.find('.button[category=' + category + ']').addClass('selected');

      container.find('.brush').removeClass('selected');           // remove the selected box from all brushes
      selectedMaterial.addClass('selected');                // select the selected brush

      container.find('.materialSubTab').hide();                   // hide all sub palettes
      selectedMaterial.closest('.materialSubTab').show();   // show the one with the selected brush
   },

   restoreState: function(state) {
      this._state = state[this._materialClass]
      if (!this._state) {
         this._state = {
            brushes: {}
         }         
      }
      this._selectBrush(this._state.brush, true);
   },

   getSelectedBrush : function() {
      if (!this._state.brush) {
         this._selectBrush();
      }
      return this._state.brush;
   },

   _addToPalette: function(toolbar, palette) {
      if (!palette) {
         return;
      }

      var self = this;
      $.each(palette, function(material, brushes) {
         var category = material.replace(' ', '_').replace(':', '_');

         if (toolbar) {
            if (!toolbar.find('img[category="' + category + '"]')[0]) {
               toolbar.append($('<img>')
                                 .addClass('button')
                                 .addClass(category)
                                 .attr('category', category)
                                 .append('<div class=selectBox />')
                              );
            }
         }
         var palette = self._container.find('.brushPalette[category="' + category + '"]');
         if (palette.length == 0) {
            var subtab = $('<div>', { id:material.category, class: 'materialSubTab' });
            subtab.html($('<h2>')
                     .text(category + ' ' + self._tabTitle))

            var downSection = $('<div>', { class:'downSection' })
            palette = $('<div>').addClass('brushPalette')
                                .attr('category', category);
            downSection.append(palette);
            subtab.append(downSection);
            self._container.append(subtab);
         }
         self._buildMaterialPalette(palette, category, brushes);
      });
   },

   _addMaterialPalette: function(colors, patterns) {     
      var self = this;      
      var toolbar;

      // add the selector...
      var toolbar = $('<div>').attr('id', 'materialToolbar')
                              .addClass('downSection');

      self._container.append(toolbar);

      // add the tabs...
      self._addToPalette(toolbar, patterns);
      self._addToPalette(toolbar, colors);

      self._container.find('.brush').click(function() {
         var brush = $(this).attr('brush');

         self._container.find('.' + self._materialClass).removeClass('selected');
         $(this).addClass('selected');

         self._selectBrush(brush);
         self._clickHandler(brush);
      });

      if (toolbar) {
         toolbar.on('click', '.button', function() {
            var category = $(this).attr('category');
            var brush = self._state.brushes[category];
            if (!brush) {
               brush = self._container.find('.brush[category=' + category + ']').attr('brush');
            }
            self._selectBrush(brush);
            self._clickHandler(brush);
         });
      }

      self._container.find("[title]").tooltipster();
   }
});

