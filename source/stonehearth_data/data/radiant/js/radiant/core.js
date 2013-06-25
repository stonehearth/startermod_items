"use strict";

var radiant = {
   _deferred: $.Deferred(),

   log: function (msg) {
      $(top).trigger("radiant.dbg.log", msg);
      console.log(msg);
   },
   ready: function (fn) {
      radiant._deferred.done(fn);
   },
   _sendReady: function () {
      radiant._deferred.resolve();
   }
};
