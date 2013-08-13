App = Ember.Application.createWithMixins({
	//rootElement: '#main',
    LOG_TRANSITIONS: true,

  	init: function() {
  		this.deferReadiness();
  		this._super();
      this._loadModules();
  	},

  	ready: function() {
      //Start the poll
      radiant.events.poll();
  	},


    _loadModules: function() {
      var self = this;

      var deferreds = [];

      $.ajax( {
        url: '/stonehearth/html/init.json'
      }).done( function(data) {
        console.log(data);

        deferreds = deferreds.concat(self._loadJavaScripts(data.modules));
        deferreds = deferreds.concat(self._loadCsss(data.modules));
        deferreds = deferreds.concat(self._loadTemplates(data.modules));

        // when all the tempalates are loading, contune loading the app
        $.when.apply($, deferreds).then(function(data) {
          self.advanceReadiness();
        });
      });
    },

    _loadJavaScripts: function(modules) {
      var deferreds = [];
      var self = this;

      $.each( modules, function( k, v ) {
        var url = '/' + k + '/html/js/' + k + ".js";
        deferreds.push(self._loadJavaScript(url));
      });

      return deferreds;
    },

    _loadJavaScript: function(url) {
      console.log('loading js ' + url);

      var deferred = $.Deferred();

      var script = document.createElement('script');
      script.type = 'text/javascript';
      script.src = url;
      script.onload = function () {
        deferred.resolve();
      };
      script.onerror = function () {
         throw ('failed to load script at ' + url);
      };
      document.getElementsByTagName('head')[0].appendChild(script);

      return deferred;
    },

    _loadCsss: function(modules) {
      var deferred = $.Deferred();

      // collect the css urls from all the modules
      var urls = [];
      $.each(modules, function( k, v ) {
        var url = '/' + k + '/html/css/' + k + ".less";
        urls.push(url);
      });

      // grab the root stylesheet
      $.get('css/style.less', function(data) {
        // append imports for all the module style sheets
        $.each(urls, function(i, url) {
          data = data + '\n' + '@import "' + url + '";';
        })

        // parse everything all at once and add the css to the page
        var parser = new(less.Parser)({
          paths: ['css/']
        }).parse(data, function (err, t) {
            if (err) {
              console.error(err) 
            }

            css = t.toCSS();
            $("<style/>").html(css).appendTo("body");

            // css has been added. Resolve the deferred
            deferred.resolve();
        });
      })

      return deferred;
    },

  	_loadTemplates: function(modules) {
  		var self = this;

      var templateUrls = [];

      $.each( modules, function( k, v ) {
        templateUrls.push('/' + k + '/html/' + k + ".html");
      });

  		var deferreds = [];

  		// make all the ajax calls and collect the deferres
  		$.each(templateUrls, function(index, templateUrl) {
  			deferreds.push(self._loadTemplate(templateUrl));
  		});

      return deferreds;
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

App.Router.map(function() {
  
});
