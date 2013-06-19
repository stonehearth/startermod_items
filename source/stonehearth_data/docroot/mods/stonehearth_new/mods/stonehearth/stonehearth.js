
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
		this.add(App.MainbarView);
		this.add(App.UnitFrameView, { remoteId: 1});
		this.add(App.CraftingWindowView, {remoteId: 1});
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
