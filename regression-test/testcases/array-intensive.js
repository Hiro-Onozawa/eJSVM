var bench = function () {
  var a = new Array(100);
  var c;
  var j;
  for (c = 0; c < 300000; c++) {
    a[0] = c;
    for (j = 0; j < a.length - 4; j++) {
      a[j + 1] = a[j];
      a[j + 2] = a[j];
      a[j + 3] = a[j];
      a[j + 4] = a[j];
    }
  }
}

bench();
