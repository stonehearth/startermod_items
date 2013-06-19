App.CraftingWindowView = App.View.extend({
	templateName: 'craftingWindow',
	components: ['unit_info', 'crafter_info'],
	
	select: function(id) {
		var self = this;
		console.log('fetching object ' + id);
		jQuery.getJSON("http://localhost/api/objects/" + id + ".txt", function(json) {
			self.set('context.current', json);
		});
	},

	craft: function() {
		console.log('craft!');
	}
});


