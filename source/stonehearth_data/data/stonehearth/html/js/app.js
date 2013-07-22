App = Ember.Application.createWithMixins({
	//rootElement: '#main',

  	init: function() {
  		this.deferReadiness();
  		this._super();
  		this._loadTemplates();
  	},

  	ready: function() {
      //Start the poll
      radiant.events.poll();
  	},


  	_loadTemplates: function() {
  		var self = this;

  		var templateUrls = [
  			'stonehearth/stonehearth.html',
         'mainbar/mainbar.html',
         'unitframe/unitframe.html',
         'objectbrowser/objectbrowser.html',
         '/stonehearth_crafter/html/stonehearth_crafter.html',
         '/stonehearth_calendar/html/stonehearth_calendar.html'
  			];

  		var deferreds = [];

  		// make all the ajax calls and collect the deferres
  		$.each(templateUrls, function(index, templateUrl) {
  			deferreds.push(self._loadTemplate(templateUrl));
  		});

  		// when all the tempalates are loading, contune loading the app
  		$.when.apply($, deferreds).then(function(data) {
			self.advanceReadiness();
  		});
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
