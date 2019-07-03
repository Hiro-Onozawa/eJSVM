var a,b,c;
a=function(){}
b=function(){}
c=function(){}
a.prototype=b.prototype=c.prototype={};
print(a.prototype);
print(b.prototype);
print(c.prototype);
