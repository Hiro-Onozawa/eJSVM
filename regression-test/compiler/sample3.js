var f = function(x) {
    var arguments = 2;
    var g = function() {
        var y = 4;
        return x + y;
    }
    return g();
}
f(5);
