$(document).ready(function(){
   var commandQueue = [];
   var runningCommand;

   var commands = {
      CLICK_DOM_ELEMENT: function(jqselector) {
         var try_click = function () {
            console.log('looking for', jqselector);
            element = $(jqselector)[0];
            if (element) {
               console.log('clicking', element);
               element.click();
               finishCommand();
            } else {
               console.error('could not find element to click! ' + jqselector);
            }
         }
         try_click();
      },
      STRESS_UI_PERFORMANCE_EASY: function() {
         var rootDiv = $('html')[0];
         maxWidth = $(window).width();
         maxHeight = $(window).height();

         var rect = document.createElement('div');
         rect.style.width = '300px';
         rect.style.height = '300px';
         rect.style.position = 'absolute';
         rect.style.backgroundColor = 'yellow';
         rect.style.left = '10px';
         rect.style.top = '10px';
         rootDiv.appendChild(rect);

         var sidewaysTime = 2000.0;
         var updownTime = 2000.0;

         var sidewaysRate = ((maxWidth - 300) / sidewaysTime) * 20;
         var updownRate = ((maxHeight - 300) / updownTime) * 20;

         var left = 10;
         var top = 10;

         // move to the right.
         var t1 = window.setInterval(function() {
            rect.style.left = left + 'px';
            left += sidewaysRate;
         }, 20);
         window.setTimeout(function() {
            window.clearInterval(t1);

            // move down.
            var t2 = window.setInterval(function() {
               rect.style.top = top + 'px';
               top += updownRate;
            }, 20);
            window.setTimeout(function() {
               window.clearInterval(t2);

               // move left.
               var t3 = window.setInterval(function() {
                  rect.style.left = left + 'px';
                  left -= sidewaysRate;
               }, 20);
               window.setTimeout(function() {
                  window.clearInterval(t3);

                  // move left.
                  var t4 = window.setInterval(function() {
                     rect.style.top = top + 'px';
                     top -= updownRate;
                  }, 20);
                  window.setTimeout(function() {
                     finishCommand();
                     window.clearInterval(t4);
                  }, updownTime);
               }, sidewaysTime);
            }, updownTime);
         }, sidewaysTime);
      },
      STRESS_UI_PERFORMANCE_HARD: function() {
         var rootDiv = $('html')[0];
         maxWidth = $(window).width();
         maxHeight = $(window).height();

         var left1 = 10;
         var top1 = 10;
         var left2 = (maxWidth - 300);
         var top2 = (maxHeight - 300);

         var rect1 = document.createElement('div');
         rect1.style.width = '300px';
         rect1.style.height = '300px';
         rect1.style.position = 'absolute';
         rect1.style.backgroundColor = 'red';
         rect1.style.left = left1 + 'px';
         rect1.style.top = top1 + 'px';
         rootDiv.appendChild(rect1);
         
         var rect2 = document.createElement('div');
         rect2.style.width = '300px';
         rect2.style.height = '300px';
         rect2.style.position = 'absolute';
         rect2.style.backgroundColor = 'red';
         rect2.style.left = left2 + 'px';
         rect2.style.top = top2 + 'px';
         rootDiv.appendChild(rect2);

         var sidewaysTime = 2000.0;
         var updownTime = 2000.0;

         var sidewaysRate = ((maxWidth - 300) / sidewaysTime) * 20;
         var updownRate = ((maxHeight - 300) / updownTime) * 20;

         // move to the right.
         var t1 = window.setInterval(function() {
            rect1.style.left = left1 + 'px';
            left1 += sidewaysRate;
            
            rect2.style.left = left2 + 'px';
            left2 -= sidewaysRate;
         }, 20);
         window.setTimeout(function() {
            window.clearInterval(t1);

            // move down.
            var t2 = window.setInterval(function() {
               rect1.style.top = top1 + 'px';
               top1 += updownRate;

               rect2.style.top = top2 + 'px';
               top2 -= updownRate;
            }, 20);
            window.setTimeout(function() {
               window.clearInterval(t2);

               // move left.
               var t3 = window.setInterval(function() {
                  rect1.style.left = left1 + 'px';
                  left1 -= sidewaysRate;
                  
                  rect2.style.left = left2 + 'px';
                  left2 += sidewaysRate;
               }, 20);
               window.setTimeout(function() {
                  window.clearInterval(t3);

                  // move left.
                  var t4 = window.setInterval(function() {
                     rect1.style.top = top1 + 'px';
                     top1 -= updownRate;
                     
                     rect2.style.top = top2 + 'px';
                     top2 += updownRate;
                  }, 20);
                  window.setTimeout(function() {
                     finishCommand();
                     window.clearInterval(t4);
                  }, updownTime);
               }, sidewaysTime);
            }, updownTime);
         }, sidewaysTime);
      }
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
