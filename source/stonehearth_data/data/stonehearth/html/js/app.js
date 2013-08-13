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
        url: '/api/get_modules'
      }).done( function(data) {
        console.log(data);

        deferreds = deferreds.concat(self._loadJavaScripts(data));
        deferreds = deferreds.concat(self._loadCsss(data));
        deferreds = deferreds.concat(self._loadTemplates(data));

        // when all the tempalates are loading, contune loading the app
        $.when.apply($, deferreds).then(function() {
          self.advanceReadiness();
        });
      });
    },

    _loadJavaScripts: function(modules) {
      var deferreds = [];
      var self = this;

      $.each( modules, function( name, data ) {
        if (data.ui && data.ui.js) {
          $.each(data.ui.js, function(i, url) {
            deferreds.push(self._loadJavaScript(url));
          });
        }
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
         console.log('failed to load script at ' + url);
         deferred.resolve();
      };
      document.getElementsByTagName('head')[0].appendChild(script);

      return deferred;
    },

    _loadCsss: function(modules) {
      var deferred = $.Deferred();

      // collect the css urls from all the modules
      var urls = [];
      $.each(modules, function( name, data ) {
        if (data.ui && data.ui.less) {
          urls = urls.concat(data.ui.less);
        }
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
      var deferreds = [];

      $.each( modules, function( name, data ) {
        if (data.ui && data.ui.html) {
          $.each(data.ui.html, function(i, templateUrl) {
            deferreds.push(self._loadTemplate(templateUrl));
          });
        }
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
