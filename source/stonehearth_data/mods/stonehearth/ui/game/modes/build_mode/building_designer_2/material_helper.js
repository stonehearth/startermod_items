var MaterialHelper = {};

MaterialHelper.index = 0;

MaterialHelper.buildMaterialPalette = function(items, materialClassName) {
   var palette = $('<div>').addClass('brushPalette');

   // for each category
   $.each(items, function(i, material) {
      var brush = $('<div>')
                     .addClass('brush')
                     .attr('brush', material.brush)
                     .css({ 'background-image' : 'url(' + material.portrait + ')' })
                     .attr('title', material.name)
                     .attr('index', MaterialHelper.index)
                     .addClass(materialClassName)
                     .append('<div class=selectBox />'); // for showing the brush when it's selected

      palette.append(brush);
      
      MaterialHelper.index += 1;
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
      var material = $(this).attr('index');

      clickHandler($(this).attr('brush'), material);
   });

   el.find("[title]").tooltipster();
};
