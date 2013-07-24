App.CraftingWindowView = App.View.extend({
	templateName: 'craftingWindow',
	components: ['unit_info', 'crafter_info', 'queue_info'],

	//Fired when the template has loaded. Continue to
	//run until all relevant children are inserted.
	//Then attach all jquery functionality to items
	didInsertElement: function() {
		if ($("#recipeAccordion").find("h3").length > 0 &&
			 $('#orders').find("div").length > 0 ) {
		  this._super();
		  this._buildAccordion();
		  this._buildOrderList();
		} else {
			console.log("craftingWindow waiting for data..." + $('#orders').find("div").length);
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
  		$( "#orders, #garbageList" ).sortable({
  			axis: "y",
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
  		var currentOrdersList = $('#orders');
		//Set the default state of the buttons
		if (currentOrdersList[0].scrollHeight > currentOrdersList.height()) {
			$('#orderListUpBtn').show();
			$('#orderListDownBtn').show();
		} else {
			$('#orderListUpBtn').hide();
			$('#orderListDownBtn').hide();
		}

		//Register an event to toggle the buttons when the scroll state changes
		currentOrdersList.on("overflowchanged", function(event){
		   console.log("overflowchanged!");
		   $('#orderListUpBtn').toggle();
		   $('#orderListDownBtn').toggle();
		});
  	},

  	//Call this function when the selected order changes.
	select: function(url) {
		var self = this;
		console.log('fetching object ' + url);
		jQuery.getJSON(url, function(json) {
			self.set('context.current', json);
		});
	},

	//Call this function when the user is ready to submit an order
	craft: function() {
		console.log('craft!');
		//TODO: replace with call to the server
		$("#orders").append('<div class="orderListItem"><a href=#>new</a></div>');
	},

	scrollOrderListUp: function() {
		this._scrollOrderList(-153)
	},

	scrollOrderListDown: function() {
		this._scrollOrderList(153)
	},

	_scrollOrderList: function(amount) {
		var orderList = $('#orders'),
		localScrollTop = orderList.scrollTop() + amount;
		orderList.animate({scrollTop: localScrollTop}, 100);
	}

});