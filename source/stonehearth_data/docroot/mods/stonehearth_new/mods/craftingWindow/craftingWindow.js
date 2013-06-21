App.CraftingWindowView = App.View.extend({
	templateName: 'craftingWindow',
	components: ['unit_info', 'crafter_info', 'queue_info'],

	//Fired when the template has loaded. Continue to
	//run until all relevant children are inserted.
	//Then attach all jquery functionality to items
	didInsertElement: function() {
		if (($("#recipeList").find("h3").length > 0) &&
			 ($('#currentOrdersList').find("li").length > 0) ) {
		  this._super();
		  this._buildAccordion();
		  this._buildOrderList();
		  console.log("hello!");
		}
		else {
		  Ember.run.next(this,function(){
		      this.didInsertElement();
		  });
		}
	},

	//Attach accordion functionality to the appropriate div
	_buildAccordion: function() {
		var accordion = $("#recipeAccordion");
		accordion.accordion({
			active: 1,
			heightStyle: "fill"
		});
  	},

  	//Attach sortable/draggable functionality to the order
  	//list. Hook order list onto garbage can. Set up scroll
  	//buttons.
  	_buildOrderList: function(){
  		$( "#currentOrdersList, #garbageList" ).sortable({
     		connectWith: "#garbageList",
     		beforeStop: function (event, ui) {
             if(ui.item[0].parentNode.id == "garbageList") {
             	ui.item.remove();
             }
         }
      }).disableSelection();
  		this._initButtonStates();
  	},

  	//Initialize button states to visible/not based on contents of
  	//list. Register an event to toggle them when they
  	//are no longer needed. TODO: animation!
  	_initButtonStates: function() {
  		var currentOrdersList = $('#currentOrdersList');
		//Set the default state of the buttons
		if (currentOrdersList[0].scrollHeight > currentOrdersList.height()) {
			$('#orderListUpBtn').css('display', 'block');
			$('#orderListDownBtn').css('display', 'block');
		} else {
			$('#orderListUpBtn').css('display', 'none');
			$('#orderListDownBtn').css('display', 'none');
		}

		//Register an event to toggle the buttons when the scroll state changes
		currentOrdersList.on("overflowchanged", function(event){
		   console.log("overflowchanged!");
		   $('#orderListUpBtn').toggle();
		   $('#orderListDownBtn').toggle();
		});

		//Register the button handlers to scroll
		this._registerButtonHandlers();

  	},

  	//On click scroll up or down
  	_registerButtonHandlers: function(){
		$('#orderListUpBtn').click(function() {
		  var orderList = $('#currentOrdersList'),
		      localScrollTop = orderList.scrollTop() - 100;
		     orderList.animate({scrollTop: localScrollTop}, 100);
		});
		$('#orderListDownBtn').click(function() {
		     var orderList = $('#currentOrdersList'),
		      localScrollTop = orderList.scrollTop() + 100;
		     orderList.animate({scrollTop: localScrollTop}, 100);
		});
  	},

  	//Call this function when the selected order changes.
	select: function(id) {
		var self = this;
		console.log('fetching object ' + id);
		jQuery.getJSON("http://localhost/api/objects/" + id + ".txt", function(json) {
			self.set('context.current', json);
		});
	},

	//Call this function when the user is ready to submit an order
	craft: function() {
		console.log('craft!');
		//TODO: replace with call to the server
		$("#currentOrdersList").append('<li class="newItem">Item3</li>');
	}
});