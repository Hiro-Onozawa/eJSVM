var x;
x = parseInt("100");
print(x);  // 100
x = parseInt(" 100");
print(x);  // 100
x = parseInt("1000000000000000000000000");
print(x);  // (double value) 1e+24
x = parseInt("1000000000000000000000000", 20);
print(x);  // (double value) 1.6777216e+31
x = parseInt("0x100", 16);
print(x);  // 0
x = parseInt("100.1");
print(x);  // 100
x = parseInt("0x100");
print(x);  // 256
x = parseInt("100", 2);
print(x);  // 4
x = parseInt("100", 36);
print(x);  // 1296
x = parseInt("100", "16");
print(x);  // 256
x = parseInt("-0x100")
print(x);  // -256
x = parseInt("-0");
print(x);  // -0
x = parseInt("");
print(x)   // Nan
x = parseInt("-");
print(x)   // Nan
x = parseInt("x");
print(x)   // Nan
x = parseInt("-  100");
print(x);  // Nan
x = parseInt("100", 37);
print(x);  // Nan
