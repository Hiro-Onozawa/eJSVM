// The Great Computer Language Shootout
// http://shootout.alioth.debian.org/
//
// modified by Isaac Gouy

pad = function (number,width){
   var s;
   var prefixWidth;
	 var i;
	 s = number.toString();
	 prefixWidth= width - s.length;
   if (prefixWidth>0){
      for (i=1; i<=prefixWidth; i++) s = " " + s;
   }
   return s;
}

nsieve = function (m, isPrime){
   var i, k, count;

   for (i=2; i<=m; i++) { isPrime[i] = true; }
   count = 0;

   for (i=2; i<=m; i++){
      if (isPrime[i]) {
         for (k=i+i; k<=m; k+=i) isPrime[k] = false;
         count++;
      }
   }
   return count;
}

sieve = function () {
		var i;
		var m;
		var flags;
    for (i = 1; i <= 3; i++ ) {
        m = (1<<i)*10000;
        flags = Array(m+1);
        nsieve(m, flags);
    }
}

sieve();
