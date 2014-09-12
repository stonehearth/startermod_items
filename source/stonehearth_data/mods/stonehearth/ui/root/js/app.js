App = Ember.Application.createWithMixins({
    LOG_TRANSITIONS: true,

  	init: function() {
      this._super();
      var self = this;

      this._getOptions();

      this.deferReadiness();
      var deferreds = []
      radiant.call('stonehearth:sign_in').done(function(r) {
        App.stonehearthVersion = r.version;
        self._loadModules()
      });
     
  	},

    getModuleData: function() {
      return this._moduleData;
    },

    _loadModules: function() {
      var self = this;

      var deferreds = [];

      radiant.call('radiant:get_modules')
         .done( function(data) {
             console.log(data);

             self._moduleData = data;

             deferreds = deferreds.concat(self._loadJavaScripts(data));
             deferreds = deferreds.concat(self._loadCsss(data));
             deferreds = deferreds.concat(self._loadTemplates(data));
             deferreds = deferreds.concat(self._loadLocales(data));

             // when all the tempalates are loading, contune loading the app
             $.when.apply($, deferreds).then(function() {
                self._loadGameServices()
             })
         .fail(function(e) {
            radiant.report_error('call to radiant.get_modules failed', e);
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
			    	console.log('loaded template:' + templateName)
			  	});
	    	}
	  	});
  	},

    _loadLocales: function(modules) {
      var self = this;
      var deferreds = [];

      $.each( modules, function( name, data ) {
        if (data.ui && (data.ui.html || data.ui.js))  {
          deferreds.push(self._loadLocale(name));
        }
      });

      return deferreds;
    },

    _loadLocale: function(namespace) {
      var deferred = $.Deferred();

      i18n.loadNamespace(namespace, function() {
        console.log('loaded locale namespace: ' + namespace);
        deferred.resolve();
      });

      return deferred;
    },

    _loadGameServices: function() {
      var self = this;
      var deferreds = [];

      // build up the array of deferreds, one for each service
      deferreds.push($.Deferred());
      App.population = new StonehearthPopulation(deferreds[deferreds.length - 1]);

      deferreds.push($.Deferred());
      App.inventory = new StonehearthInventory(deferreds[deferreds.length - 1]);

      deferreds.push($.Deferred());
      App.bulletinBoard = new StonehearthBulletinBoard(deferreds[deferreds.length - 1]);

      // xxx, move the calendar to a game service!       

      $.when.apply($, deferreds).then(function() {
        self.advanceReadiness();
      })
    },

    // parse the querystring into a map of options
    _getOptions: function() {
      var vars = [], hash;
      var hashes = window.location.href.slice(window.location.href.indexOf('?') + 1).split('&');

      console.log('ui options');
      console.log('--------------------')
      for(var i = 0; i < hashes.length; i++)
      {
          hash = hashes[i].split('=');
          vars.push(hash[0]);
          vars[hash[0]] = hash[1];
          console.log(hash[0] + ": " + hash[1]);
      }
      console.log('--------------------')

      this.options = vars;
    },

  	start: function() {
  		// TODO, hook this up. It's just prettier that way
  	}
});

App.stonehearth = {};

App.Router.map(function() {

});
