/*

Calls i18n on the title before passing into tooltipster.

*/
(function ($) {

   $.fn.i18ntooltipster = function() {

      return this.each(function () {
         var elem = $(this);
         if (!elem.data('tooltipster')) {
            var title = elem.attr('title');
            if(typeof title === 'undefined') {
               return;
            }
            var translated_title = i18n.t(title);
            elem.tooltipster({content: translated_title});
         }
      });

   };

}( jQuery ));