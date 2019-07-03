o = new Object();
o.toString = function () { return "soko"; };
print("abc" + o);
o.x = 100
f = function() { return this.x; };
x = "abc";
print(f());
print(f.apply(o, []));
g = function(u, v) { return this.x + u + v; };
print(g(99, 10000));
print(g.apply);
print(g.apply(o, [99, 10000]));
max = Math.max;
print(max(2000,3000));
print(max.apply);
print(max.apply(o, [2000,3000]));
