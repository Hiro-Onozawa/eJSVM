var f = function(a, b) {
    var g = function() {
        print(a);
        print(y);
    }
    var x = 6, y = 7;
    print(b);
    print(x);
    g();
}
f(4, 5);
