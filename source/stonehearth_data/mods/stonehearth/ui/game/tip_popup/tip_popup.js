var stonehearthTipData = {};

$(document).ready(function(){

   $(top).on('radiant_show_tip', function (_, data) {
      destroyCurrentTip();
      stonehearthTipData.currentTip = App.gameView.addView(App.StonehearthTipPopup, data);
   });

   $(top).on('radiant_hide_tip', function (_, e) {
      destroyCurrentTip();
      //$(top).trigger('radiant_show_main_actionbar');
   });

   var destroyCurrentTip = function() {
      if (stonehearthTipData.currentTip) {
         stonehearthTipData.currentTip.destroy();
         stonehearthTipData.currentTip = null;
      }
   }

});

App.StonehearthTipPopup = App.View.extend({
   templateName: 'stonehearthTipPopup',

   didInsertElement: function() {
      this._super();

      var css = this.get('css');
      if (css) {
         $('#tipPopup').css(css);
      }

      var cssClass =this.get('cssClass')
      if (cssClass) {
         $('#tipPopup').addClass(cssClass);
      }

      console.log(this.element);
      console.log($('#tipPopup'));
      $('#tipPopup').pulse();
   }
});

