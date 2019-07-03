var bench = function() {
  var a = "abc"
  var b = "100"

  var i = 0
  var j = 0

  for (j = 0; j < 10000; j++) {
    for (i = 0; i < 300; i++) {
      if (b == i) {
        a = a + b;
      }
      if (a == b) {
        print('err')
      }
    }
  }
}

bench();
