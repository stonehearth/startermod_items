var stonehearthTipData = {};

$(document).ready(function(){

   $(top).on('show_tip.radiant', function (_, data) {
      destroyCurrentTip();

      $(top).trigger('hide_main_actionbar.radiant');
      stonehearthTipData.currentTip = App.gameView.addView(App.StonehearthTipPopup, { 
         context: {
            title: data.title,
            description: data.description
         }
      });

   });

   $(top).on('hide_tip.radiant', function (_, e) {
      destroyCurrentTip();
      $(top).trigger('show_main_actionbar.radiant');
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
      console.log(this.element);
      console.log($('#tipPopup'));
      $('#tipPopup').pulse();
   }
});

