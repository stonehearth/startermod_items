"use strict";

var radiant = {
   _deferred: $.Deferred(),

   log: {
      info : function (msg) {
         console.log(msg);
      },
      warning : function (msg) {
         console.log(msg);
      },
   },

   report_error: function(msg, obj) {
      console.error(msg, obj);
   },

   assert: function(condition, message) {
      if (!condition) {
         throw message || "assertion failed."
      }
   }
};
