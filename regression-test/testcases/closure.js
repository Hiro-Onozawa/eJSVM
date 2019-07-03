var f = function(x){
var g = function(){
x = x + 1;
return x;};
return g;
};
var k = f(10);
print(k());
