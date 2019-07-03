// The Great Computer Language Shootout
//  http://shootout.alioth.debian.org
//
//  Contributed by Ian Osgood

var last = 42;
var A = 3877;
var C = 29573;
var M = 139968;

var rand = function(max) {
  last = (last * A + C) % M;
  return max * last / M;
}

var ALU =
  "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" +
  "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" +
  "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" +
  "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" +
  "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" +
  "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" +
  "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

var IUB = {
  a:0.27, c:0.12, g:0.12, t:0.27,
  B:0.02, D:0.02, H:0.02, K:0.02,
  M:0.02, N:0.02, R:0.02, S:0.02,
  V:0.02, W:0.02, Y:0.02
}

var HomoSap = {
  a: 0.3029549426680,
  c: 0.1979883004921,
  g: 0.1975473066391,
  t: 0.3015094502008
}

var makeCumulative = function(table) {
  var last = null;
  var c;
  for (c in table) {
    if (last) table[c] += table[last];
    last = c;
  }
}
    
var fastaRepeat = function(n, seq) {
    var seqi = 0;
    var lenOut = 60;
    var s;
    while (n>0) {
	if (n<lenOut) lenOut = n;
	if (seqi + lenOut < seq.length) {
	    ret = seq.substring(seqi, seqi+lenOut);
	    seqi += lenOut;
	} else {
	    s = seq.substring(seqi);
	    seqi = lenOut - s.length;
	    ret = s + seq.substring(0, seqi);
	}
	n -= lenOut;
  }
}

var fastaRandom = function (n, table) {
  var line = new Array(60);
  var i;
  var r;
  makeCumulative(table);
  while (n>0) {
    if (n<line.length) line = new Array(n);
    for (i=0; i<line.length; i++) {
      r = rand(1);
      for (c in table) {
        if (r < table[c]) {
          line[i] = c;
          break;
        }
      }
    }
    ret = line.join('');
    n -= line.length;
  }
}

var ret;

var count = 7;
ret = fastaRepeat(2*count*100000, ALU);
ret = fastaRandom(3*count*1000, IUB);
ret = fastaRandom(5*count*1000, HomoSap);

