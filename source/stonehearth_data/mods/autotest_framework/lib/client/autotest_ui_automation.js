$(document).ready(function(){
   var commandQueue = [];
   var runningCommand;

   var commands = {
      CLICK_DOM_ELEMENT: function(jqselector) {
         console.log('looking for', jqselector);
         var try_click = function () {
            element = $(jqselector)[0];
            if (element) {
               console.log('clicking', element);
               window.setTimeout(function () {
                  element.click();
                  finishCommand();
               }, 200);
            } else {
               window.setTimeout(try_click, 100)
            }
         }
         try_click();
      },
   }

   var finishCommand = function() {
      runningCommand = null;
   }

   var pumpCommandQueue = function() {
      if (!runningCommand && commandQueue.length > 0) {
         runningCommand = commandQueue.shift();
         console.log('automation dispatching ', runningCommand);
         commands[runningCommand[0]].apply(null, runningCommand.slice(1));
      }
   }
   window.setInterval(pumpCommandQueue, 100);

   radiant.call('autotest_framework:ui:connect_browser_to_client')
            .progress(function (cmd) {
               // an array.  looks somethign like ['CLICK_DOM_ELEMENT', '#someButton']
               console.log('automation queuing ', cmd, 'current length is', commandQueue.length);
               commandQueue.push(cmd);
            });
});
