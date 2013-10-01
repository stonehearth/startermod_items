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

   $.fn.trueScale = function(options) { 
      return $(this).animate(
      {  effect: 100 }, 
      {
         duration: options.duration,
         step: function(now,fx) {
            var maxScale = options.scale;
            var scalePercent = 100 - now;
            var scale = 1 + ((maxScale - 1) * scalePercent/100)
            console.log(scale);
            $(this).css('-webkit-transform','scale('+scale+','+scale+')'); 
         }
      });
   }   
})(jQuery);