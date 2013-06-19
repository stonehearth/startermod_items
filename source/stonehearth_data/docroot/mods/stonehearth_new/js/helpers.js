Handlebars.registerHelper('i18n', function(i18n_key) {
  var result = i18n.t(i18n_key);

  return new Handlebars.SafeString(result);
});

Handlebars.registerHelper('tr', function(context, options) { 
   var opts = i18n.functions.extend(options.hash, context);
   if (options.fn) opts.defaultValue = options.fn(context);
 
   var result = i18n.t(opts.key, opts);
 
   return new Handlebars.SafeString(result);
});
