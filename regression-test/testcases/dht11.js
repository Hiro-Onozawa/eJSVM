function send_request(DHTPIN) {
  write_gpio(DHTPIN, 1);
//  Time.delay(50);
  write_gpio(DHTPIN, 0);
//  Time.delay(20);
}

function read_signal(DHTPIN) {
  var MAX_UNCHANGE_COUNT = 100;
  var unchanged_count = 0;
  var last = -1;
  var data = [];
  while (true) {
    var current = read_gpio(DHTPIN);
    data.push(current);
    if (last != current) {
      unchanged_count = 0;
      last = current;
    } else {
      unchanged_count += 1;
      if (unchanged_count > MAX_UNCHANGE_COUNT) {
        break;
      }
    }
  }
  return data;
}

function get_lengths_from_signal(data) {
  var STATE_INIT_PULL_DOWN        = 1;
  var STATE_INIT_PULL_UP          = 2;
  var STATE_DATA_FIRST_PULL_DOWN  = 3;
  var STATE_DATA_PULL_UP          = 4;
  var STATE_DATA_PULL_DOWN        = 5;

  var state = STATE_INIT_PULL_DOWN;
  var lengths = [];
  var current_length = 0;
  for (var i = 0; i < data.length; i++) {
    var current = data[i];
    current_length += 1;

    if (state == STATE_INIT_PULL_DOWN) {
      if (current == 0) {
        state = STATE_INIT_PULL_UP;
      } else {
        continue;
      }
    }
    if (state == STATE_INIT_PULL_UP) {
      if (current == 1) {
        state = STATE_DATA_FIRST_PULL_DOWN;
      } else {
        continue;
      }
    }
    if (state == STATE_DATA_FIRST_PULL_DOWN) {
      if (current == 0) {
        state = STATE_DATA_PULL_UP;
      } else {
        continue;
      }
    }
    if (state == STATE_DATA_PULL_UP) {
      if (current == 1) {
        current_length = 0;
        state = STATE_DATA_PULL_DOWN;
      } else {
        continue;
      }
    }
    if (state == STATE_DATA_PULL_DOWN) {
      if (current == 0) {
        lengths.push(current_length);
        state = STATE_DATA_PULL_UP;
      } else {
        continue;
      }
    }
  }
  return lengths;
}


function max_len(ary) {
  if (ary.length > 0) {
    var x = ary[0];
    for (var i = 0; i < ary.length; i++) {
      if (x < ary[i]) x = ary[i];
    }
    return x;
  } else {
    return undefined;
  }
}

function min_len(ary) {
  if (ary.length > 0) {
    var x = ary[0];
    for (var i = 0; i < ary.length; i++) {
      if (x > ary[i]) x = ary[i];
    }
    return x;
  } else {
    return undefined;
  }
}

function make_bits_from_lengths(lengths) {
  var shortest_pull_up = min_len(lengths);
  var longest_pull_up = max_len(lengths);
  var halfway = (longest_pull_up + shortest_pull_up) / 2;

  var bits = [];
  for (var i = 0; i < lengths.length; i++) {
    var length = lengths[i];
    var bit = 0;
    if (length > halfway) {
      bit = 1;
    }
    bits.push(bit);
  }
  return bits;
}

function bytes_from_bits(bits) {
  var the_bytes = [];
  var bt = 0;
  for (var i = 0; i < bits.length; i++) {
    bt = bt << 1;
    if (bits[i]) {
      bt = bt | 1;
    } else {
      bt = bt | 0;
    }
    if ((i+1)%8==0) {
      the_bytes.push(bt);
      bt = 0;
    }
  }
  return the_bytes;
}

function parse_signal(data) {
  var lengths = get_lengths_from_signal(data);
  if (lengths.length != 40) {
    print("Data not good, skip (length : " + lengths.length + ")");
    return undefined;
  }
  var bits = make_bits_from_lengths(lengths);
  return bytes_from_bits(bits);
}



function read_dht11_dat(DHTPIN) {

  // send request signal to dht11
  send_request(DHTPIN);

  // read the signal
  var data = read_signal(DHTPIN);

  // parse the signal
  var the_bytes = parse_signal(data);
  if (the_bytes == undefined) {
    return undefined;
  }

  // check sum
  var checksum = (the_bytes[0] + the_bytes[1] + the_bytes[2] + the_bytes[3]) & 0xFF;
  if (the_bytes[4] != checksum) {
    print("Data not good, skip (checksum : " + checksum + " vs the_bytes[4] : " + the_bytes[4] + ")");
    return undefined;
  }

  return { 'humidity': the_bytes[0], 'temperature': the_bytes[2] };
}


var DHTPIN   = 23;

function calc_discomfort_index(h, t) {
  return 0.81*t + 0.01*h*(0.99*t-14.3)+46.3;
}

function discomfort_level(d) {
  var level = 0;
  if (d < 55) {
    level = 1;
  } else if (d < 60) {
    level = 2;
  } else if (d < 65) {
    level = 3;
  } else if (d < 70) {
    level = 4;
  } else if (d < 75) {
    level = 5;
  } else if (d < 80) {
    level = 6;
  } else if (d < 85) {
    level = 7;
  } else {
    level = 8;
  }
  return level;
}

function mainloop() {
  var result = read_dht11_dat(DHTPIN);
  if (result != undefined) {
    var h = result['humidity'];
    var t = result['temperature'];
    var d = calc_discomfort_index(h, t);
    var level = discomfort_level(d);

    print('humidity: '+h+',  temperature: '+t+',  discomfort: '+d+',  level: '+level);
  }
  else {
    print("Undefined value")
  }
//  Time.delay(2000);
}

function init() {
//  Raspi.init();
}

function main() {
  init();
  var i=1000;
  while (--i) {
    mainloop();
  }
}

main();


