function add_self(x) {
    var n = this;
    for (var i = 0; i < arguments.length; i++) {
	n = n + arguments[i];
    }
    return n;
}

var n = add_self.apply(10, [20, 30, 40]);
print(n);  // 100

var o = new Object();
o.f = function() {
    print("abc"); // not executed
}

print(o.f.apply.apply);  // builtin
print(o.toString.apply.apply); // builtin

var n = o.f.apply.apply(
    "xyz".toString.apply.apply,
    [add_self, [10, [20, 30, 40]]])
print(n);  // 100

