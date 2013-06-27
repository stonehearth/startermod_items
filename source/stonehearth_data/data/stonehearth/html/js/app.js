App = Ember.Application.createWithMixins({
	//rootElement: '#main',

  	init: function() {
  		this.deferReadiness();
  		this._super();
  		this._loadMods();
  	},

  	ready: function() {
      //Start the poll
      radiant.events.poll();
  	},


  	_loadMods: function() {
  		var self = this;

  		var modUrls = [
  			'stonehearth',
         'mainbar',
         'unitframe',
         'objectbrowser'
  			];

  		var deferreds = [];

  		// make all the ajax calls and collect the deferres
  		$.each(modUrls, function(index, modUrl) {
  			deferreds.push(self._loadMod(modUrl));
  		});

  		// when all the tempalates are loading, contune loading the app
  		$.when.apply($, deferreds).then(function(data) {
			self.advanceReadiness();
  		});
  	},

  	_loadMod: function(url) {
  		var modName = url.substr(url.lastIndexOf('/') + 1);
  		console.log('loading mod: ' + modName);

  		//load the mod
  		//this._loadCss(url + '/' + modName + '.less')
		return this._loadTemplate(url + '/' + modName + '.html');
  	},

	_loadCss: function (url) {
		var link = $("<link>");
		link.attr({
			type: 'text/css',
			rel: 'stylesheet',
			href: url
		});
		$("head").append(link);
	},


  	_loadTemplate: function(url) {
		return $.ajax({
	      	url: url,
	      	dataType: 'text',
	      	success: function (response) {
				$(response).filter('script[type="text/x-handlebars"]').each(function() {
			    	templateName = $(this).attr('data-template-name');
			    	Ember.TEMPLATES[templateName] = Ember.Handlebars.compile($(this).html());
			    	console.log('  loaded template:' + templateName)
			  	});


	    	}
	  	});
  	},

  	start: function() {
  		// TODO, hook this up. It's just prettier that way
  	}
});
