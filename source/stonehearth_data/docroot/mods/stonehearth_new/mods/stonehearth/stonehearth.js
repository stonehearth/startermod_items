
App.MainView = Ember.View.extend({
    show: function() {
        var containerView = Em.View.views['my_container_view']
        var childView = containerView.createChildView(App.AnotherView);
        containerView.get('childViews').pushObject(childView);
    }
});

App.StonehearthView = Ember.ContainerView.extend({
	init: function() {
		this._super();
		this.add(App.MainbarView, {url: "http://localhost/api/available_workshops.txt"});
		this.add(App.UnitFrameView, { url: "http://localhost/api/objects/1.txt"});
		this.add(App.CraftingWindowView, { url: "http://localhost/api/objects/1.txt"});
		//App.render("unitFrame");
	},

	add: function(type, options) {
		var childView = this.createChildView(type, {
			classNames: ['sh-view']
		});

		childView.setProperties(options);
		this.pushObject(childView)
	}

});
