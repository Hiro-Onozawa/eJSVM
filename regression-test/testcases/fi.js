var a;
a = {hoge:1, fuga:2};
print(a);
for (b in a){
print(b, a[b]);
}
var f = function(o){
	for (c in o){
		print(c + ":" + o[c]);
	}
}
f(a);
