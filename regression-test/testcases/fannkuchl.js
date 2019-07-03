
var fannkuch = function(n) {
   var check;
   var perm;
   var perm1;
	 var perm0;
   var count;
   var maxPerm;
   var maxFlipsCount;
   var m;
	 var i;
   var r;
	 var s;
	 var flipsCount;
	 var k;
	 var k2;
	 var temp;
	 var j;
	 var f;
   check = 0;
   perm = Array(n);
   perm1 = Array(n);
   count = Array(n);
   maxPerm = Array(n);
   maxFlipsCount = 0;
   m = n - 1;
	 f = true;

   for (i = 0; i < n; i++) {perm1[i] = i;}
	 r = n;
   while (true) {
      // write-out the first 30 permutations
      if (check < 30){
         s = "";
         for(i=0; i<n; i++) {
					s = s + (perm1[i]+1).toString();
				 }
         check++;
      }
      while (!(r == 1)) { count[r - 1] = r; r = r - 1; }
      if (!(perm1[0] == 0 || perm1[m] == m)) {
         for (i = 0; i < n; i++) perm[i] = perm1[i];

         flipsCount = 0;
         while (!((k = perm[0]) == 0)) {
            k2 = (k + 1) >> 1;
            for (i = 0; i < k2; i++) {
               temp = perm[i]; perm[i] = perm[k - i]; perm[k - i] = temp;
            }
            flipsCount++;
         }

         if (flipsCount > maxFlipsCount) {
            maxFlipsCount = flipsCount;
            for (i = 0; i < n; i++) maxPerm[i] = perm1[i];
         }
      }
			f = true;
      while (f) {
         if (r == n) return maxFlipsCount;
         perm0 = perm1[0];
         i = 0;
         while (i < r) {
            j = i + 1;
            perm1[i] = perm1[j];
            i = j;
         }
         perm1[r] = perm0;

         count[r] = count[r] - 1;
         if (count[r] > 0){
					f = false;
					}else{
         r = r + 1;
				 }
      }
   }
}

var n = 9;
var ret;
ret = fannkuch(n);

