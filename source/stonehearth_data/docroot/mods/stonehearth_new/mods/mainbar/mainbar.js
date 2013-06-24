App.MainbarView = App.View.extend({
	templateName: 'mainbar',
	components: [],

   currentMenu: null,

	didInsertElement: function() {
      var el = $(this.get('element'));

      var self = this;
      el.find('.mainButton').hover(
         function () {
            $(self.get('element')).find('#help')
               .html("<nobr>" + $(this).attr("help") + "</nobr>");
            self.showHelp(true);
         },
         function () {
            $(self.get('element')).find('#help')
               .html("&nbsp;");
            self.showHelp(false);
         }
      );

      $('#craftingMenu').hide();

      /*
    	$('[title]').tooltip({
         delay: { show: 800 }
      });
      */
    	//XXX make this AOP and use a popover instead. http://twitter.github.io/bootstrap/javascript.html#tooltips
   },

   toggleMenu: function(menu) {
      console.log('toggling' + menu);
   },

   showMenu: function(menu) {
      $('#craftingMenu').toggle();
      //this.set('showHelp', !this.get('showHelp'));
      //$('#element').tooltip('hide')
      var ctx = this.get('context');
      console.log(ctx);
      console.log(ctx.get('workshops'));
   },

   showHelp: function(show) {
      if(show) {
         $(this.get('element')).find('#help').show();
      } else {
         $(this.get('element')).find('#help').hide();
      }
   },

   buildWorkshop: function() {
      console.log("build the workshop");
      $('#craftingMenu').toggle();
   },


});