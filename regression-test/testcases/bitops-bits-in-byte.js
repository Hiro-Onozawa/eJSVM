// Copyright (c) 2004 by Arthur Langereis (arthur_ext at domain xfinitegames, tld com)


// 1 op = 2 assigns, 16 compare/branches, 8 ANDs, (0-8) ADDs, 8 SHLs
// O(n)
var bitsinbyte = function(b) {
    var m = 1;
var c = 0;
while(m<0x100) {
if(b & m) c++;
m = m << 1;
}
return c;
}

var TimeFunc = function (func) {
var x, y, t;
for(x=0; x<20000; x++)
for(y=0; y<256; y++) func(y);
}

TimeFunc(bitsinbyte);
