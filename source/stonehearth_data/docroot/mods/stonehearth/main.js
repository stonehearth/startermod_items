
var views = {};

$(function () {

   radiant.ready(function () {

      console.log(window.sh);

      radiant.views.add('stonehearth.views.unitframe');
      radiant.views.add('stonehearth.views.sidebar');

      //radiant.views.add('stonehearth.views.craftingWindow');

      // start polling for events from the client
      radiant.api.poll();
   });
});
