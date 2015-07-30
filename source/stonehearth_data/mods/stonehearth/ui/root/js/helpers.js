Handlebars.registerHelper('i18n', function(i18n_key, options) {

   var view = options.data.view;
   var attrs = options.hash;

   $.each(Ember.keys(attrs), function (i, key) {
      attrs[key] = view[attrs[key]];
   });

   var result = i18n.t(i18n_key, attrs);

   return new Handlebars.SafeString(result);
});

var stonehearth_translate = function(key, options) {
   if (key.indexOf('i18n(') == 0) {
      key = key.substr(5, key.length-6);
   }
   return i18n.translate(key, options);
}

Ember.Handlebars.registerBoundHelper('i18n_key', function(key, options) {
   if (key === undefined || key===null || key == "") {
      // If there's nothing to translate, bail.
      return null;
   }
   var attrs = options.hash;
   var result = stonehearth_translate(key, attrs);
   return new Ember.Handlebars.SafeString(result);
});


Handlebars.registerHelper('tr', function(context, options) { 
   var opts = i18n.functions.extend(options.hash, context);
   if (options.fn) opts.defaultValue = options.fn(context);
 
   var result = i18n.t(opts.key, opts);
 
   return new Handlebars.SafeString(result);
});
