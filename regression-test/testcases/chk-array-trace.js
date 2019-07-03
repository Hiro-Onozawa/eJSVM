function f() {
  var a = new Array(100);
  for (var i = 0; i < 100; i++) {
    a.push(0);
  }
  return a;
}

o = new Object();
o.hello = 'Hello';

for (var i = 0; i < 10000; i++) {
  f();
}

print(o.hello);

