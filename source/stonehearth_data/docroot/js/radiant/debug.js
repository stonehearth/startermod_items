
radiant.debug = {
   INFO: 0,
   WARNING: 1,
   ERROR: 2,

   log: function (level, msg) {
      console.log(msg);
   }

};

function radInfo(msg) {
   radiant.debug.log(radiant.debug.INFO, msg);
}
