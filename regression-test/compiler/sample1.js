var f = function(x) {
    var arguments = 2;
    var g = function() {
        return x;
    }
    return g();
}
f(5);
