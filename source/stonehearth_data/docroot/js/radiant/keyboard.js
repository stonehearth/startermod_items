console.log('loading radiant.keyboard')


// When a DOM element is added to the page, track any keybinds in it's elements
$(document).bind('DOMNodeInserted', function (event) {
   var id = event.target.id;
   if (id != undefined) {
      var el = $("#" + id);
      radiant.keyboard.addView(el);
   }
});


radiant.keyboard = {

   cmdInfo: null,
   _elementsWithHotkeys: {},

   KEY_DOWN: "radiant-keydown",

   init: function () {
      console.log("radiant.keyboard init");

      //jQuery(document).bind('keydown', 'f1', function (evt) { jQuery('#_f1').addClass('dirty'); return false; });

      // Grab the keybinds data and lisent on keydowns
      $.getJSON("conf/keybindings.json", function (data) {
         radiant.keyboard.cmdInfo = data.keybindings;

         for (var key in data.keybindings) {
            if (data.keybindings.hasOwnProperty(key)) {
               var binding = data.keybindings[key];
               console.log("Adding bind for " + binding.name + " " + binding.bind);

               
               $(document).bind('keydown', binding.bind, function (e) {
                  radiant.keyboard.onKeybind(e);
                  return false;
               });
            }
         }

         // XXX, hacky
         console.log("sending ready!");
         radiant._sendReady();
      });

      /*

      $(top.document).keydown(function (e) {
         
         var event = jQuery.Event(radiant.keyboard.KEY_DOWN);
         event.keyCode = e.keyCode;

         var focus = radiant.views.getFocus();
         if (focus != null) {
            // send the keydown event just to the focused view
            focusedElement = $("#" + focus);
            focusedElement.trigger(event);
         } else {
            // send the keydown event to all views
            radInfo("broadcasting " + radiant.keyboard.KEY_DOWN);
            for (var i = 0; i < radiant.views.views.length; i++) {
               radiant.viewManager.views[i].trigger(event);
            }
         }

      })
      */
   },

   getKeybind: function (actionId) {
      return this.cmdInfo[actionId].bind;
   },


   addKeyHandler: function (keyFunction) {
      $(top.document).keydown(function (e) { keyFunction(e) });
   },

   addView: function (v) {
      v.find("[keybind]").each(function (i) {
         var el = $(this);
         var key = radiant.keyboard.getKeybind(el.attr("keybind"));

         if (!radiant.keyboard._elementsWithHotkeys.hasOwnProperty(key)) {
            radInfo("adding keyboard hotkey for " + el.attr("id"));
            radiant.keyboard._elementsWithHotkeys[key] = el;
            el.radiantTooltip();
         }
      });
   },

   onKeybind: function (e) {
      // supress all keys if a modal dialog is up
      if ($(".ui-widget-overlay").length > 0) {
         return;
      }

      var el = radiant.keyboard._elementsWithHotkeys[e.data];

      if (el != undefined) {
         el.click();
      }
   }
}

radiant.keyboard.init();
