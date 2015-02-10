var MaterialHelper = {};

MaterialHelper.buildMaterialPalette = function(items, materialClassName) {
   var palette = $('<div>').addClass('brushPalette');

   // for each category
   $.each(items, function(i, material) {
      var brush = $('<div>')
                     .addClass('brush')
                     .attr('brush', material.brush)
                     .css({ 'background-image' : 'url(' + material.portrait + ')' })
                     .attr('title', material.name)
                     .addClass(materialClassName)
                     .append('<div class=selectBox />'); // for showing the brush when it's selected

      palette.append(brush);
   });

   return palette;
};

MaterialHelper.addMaterialTab = function(root, tabId) {
   var tab = $('<div>', {id:tabId, class: 'tabPage'});
   root.append(tab);
   return tab; 
};

MaterialHelper.addMaterialPalette = function(el, tabTitle, materialClass, materials, clickHandler) {
   $.each(materials, function(i, material) {
      el.append($('<h2>')
         .text(material.category + ' ' + tabTitle))
         .append($('<div>', {class:'downSection'})
         .append(MaterialHelper.buildMaterialPalette(material.items, materialClass))
      );
   });

   el.find('.' + materialClass).click(function() {
      el.find('.' + materialClass).removeClass('selected');
      $(this).addClass('selected');

      clickHandler($(this).attr('brush'));
   });

   el.find("[title]").tooltipster();
};
