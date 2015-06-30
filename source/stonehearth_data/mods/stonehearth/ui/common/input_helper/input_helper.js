/***
 * A class for handling inputs. It registers for listening to keyboard enter or input unfocus
 * and gives a callback when either case happens.
 ***/
var StonehearthInputHelper;

(function () {
   StonehearthInputHelper = SimpleClass.extend({

      init: function(inputElement, inputChangedCallback) {
         var self = this;
         inputElement.inputHelper = self;
         inputElement
            .keypress(function (e) {
               if (e.which == 13) {
                  $(this).blur();
              }
            })
            .blur(function (e) {
               inputChangedCallback($(this).val());
            })
            .tooltipster({content: i18n.t('input_text_tooltip')});
      },

   });
})();
