// from discourse/routes/application_routes.js
App.Route.buildRoutes(function() {
	var router = this;

	this.route("game");
	
/*
  // Topic routes
  this.resource('topic', { path: '/t/:slug/:id' }, function() {
    this.route('fromParams', { path: '/' });
    this.route('fromParams', { path: '/:nearPost' });
    this.route('bestOf', { path: '/best_of' });
  });

  // Generate static page routes
  Discourse.StaticController.pages.forEach(function(p) {
    router.route(p, { path: "/" + p });
  });

  // List routes
  this.resource('list', { path: '/' }, function() {
    router = this;

    // Generate routes for all our filters
    Discourse.ListController.filters.forEach(function(filter) {
      router.route(filter, { path: "/" + filter });
      router.route(filter, { path: "/" + filter + "/more" });
    });

    // the homepage is the first item of the 'top_menu' site setting
    var homepage = PreloadStore.get('siteSettings').top_menu.split("|")[0].split(",")[0];
    this.route(homepage, { path: '/' });

    this.route('categories', { path: '/categories' });
    this.route('category', { path: '/category/:slug/more' });
    this.route('category', { path: '/category/:slug' });
  });

  // User routes
  this.resource('user', { path: '/users/:username' }, function() {
    this.route('activity', { path: '/' });
    this.resource('preferences', { path: '/preferences' }, function() {
      this.route('username', { path: '/username' });
      this.route('email', { path: '/email' });
    });
    this.route('privateMessages', { path: '/private-messages' });
    this.route('invited', { path: 'invited' });
  });
*/
});


App.IndexRoute = Ember.Route.extend({
	/*
	renderTemplate: function() {
		this.render('unitFrame', {
			outlet: 'left'
		});
	},

	model: function() {
		var model = App.UnitFrameModel.create();
		model.get('/api/1.txt');
		return model;
	}
	*/
})
