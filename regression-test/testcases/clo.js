var f;
var g;
f = function(){
	return function(){ return 1};};
g = function(){return 2;};
var i = f(g());
