var a = [1,4,2,2];
var f = function(x,y){
    if(x < y){
	return -1
    }
    if(x > y){
	return 1;
    }
    return 0;
}
a.sort(f);
print(a[3]);
