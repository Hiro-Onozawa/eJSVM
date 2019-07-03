function outer() {
    var y = 4;
    var f = function() {
        var a = 9, x = 19;
      	var g = function() {
        	return y;
      	}
        return g();
    }
    return f();
}
outer();
