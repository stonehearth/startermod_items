var stonehearthTipData = {};

App.StonehearthTipPopup = App.View.extend({
   templateName: 'stonehearthTipPopup',
   classNames: ['fullScreen', 'flex'],

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

      console.log('showing tip: ' + this.title);
      //$('#tipPopup').pulse();
      $('#tipPopup').fadeIn();
   },

   destroy: function() {
      console.log('destroying tip: ' + this.title);
      this._super();
   }
});

