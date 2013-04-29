(function ($) {

   $.widget("stonehearth.distress", {
      options: {
         type: "parchment"
      },

      _create: function () {
         var layer = $("<div></div>");

         layer.css({
            background: "rgba(255, 0, 0, 0.5)",
            backgroundImage: "url(css/default/images/textures/distress/" + this.options.type + ".png)",
            position: "absolute",
            left: 0,
            top: 0,
            right: 0,
            bottom: 0
         });

         if (this.element.css("position") == "static") {
            this.element.css("position", "relative");
         }

         layer.scale9Grid({
            top: 10,
            bottom: 10,
            left: 10,
            right: 10
         });

         this.element.append(layer);
      },

      destroy: function () {

      },

      _setOption: function (option, value) {
         $.Widget.prototype._setOption.apply(this, arguments);
         switch (option) {

         }
      }

   });

})(jQuery);