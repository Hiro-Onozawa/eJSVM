var z10 = "0000000000";
var z20 = z10 + z10;
var z40 = z20 + z20;
var z80 = z40 + z40;
var z160 = z80 + z80;
var z320 = z160 + z160;
var z640 = z320 + z320;
var z1280 = z640 + z640;
var z2560 = z1280 + z1280;

var x;
x = parseFloat("1");
print(x); // 1
x = parseFloat("-1");
print(x); // -1
x = parseFloat("1.25");
print(x); // 1.25
x = parseFloat(".25");
print(x); // 0.25
x = parseFloat("1e2");
print(x); // 100
x = parseFloat("1000e-2");
print(x); // 10
x = parseFloat("1" + z2560 + "e-2560");
print(x); // 1
x = parseFloat(".e3");
print(x); // nan


x = parseFloat("1.2");
print(x);
x = parseFloat("2.6");
print(x);
