(function ($) {
   $.fn.pulse = function() { 
      return $(this).animate(
      {  bounce: 100 }, 
      {
         duration: 200,
         step: function(now,fx) {
            var scale = now < 50 ? now : 100 - now;
            scale *= .01;
            scale += 1;
            $(this).css('-webkit-transform','scale('+scale+','+scale+')'); 
         }
      });
   }
})(jQuery);