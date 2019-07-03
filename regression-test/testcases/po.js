var a = {a:1, b:2};
var f = function(){};
f.prototype = a;
var b;
b = new f();
b.c=3;
for(n in b){
    print(b[n]);
}
