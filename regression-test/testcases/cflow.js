// The Computer Language Shootout
// http://shootout.alioth.debian.org/
// contributed by Isaac Gouy

var ack = function (m,n){
   if (m==0) { return n+1; }
   if (n==0) { return ack(m-1,1); }
   return ack(m-1, ack(m,n-1) );
}

var fib = function (n) {
    if (n < 2){ return 1; }
    return fib(n-2) + fib(n-1);
}

var tak = function (x,y,z) {
    if (y >= x) return z;
    return tak(tak(x-1,y,z), tak(y-1,z,x), tak(z-1,x,y));
}

    var i;
for ( i = 3; i <= 7; i++ ) {
    ack(3,i);
    fib(17.0+i);
    tak(3*i+3,2*i+2,i+1);
}
