App.MainbarView = App.View.extend({
	templateName: 'mainbar',
	components: [],

	didInsertElement: function() {
    	$('[title]').tooltip();
    	//XXX make this AOP and use a popover instead. http://twitter.github.io/bootstrap/javascript.html#tooltips
  	},

});