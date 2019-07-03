/*
 * mailing list No.00327
 * This test checks that no destroyed registers by `call` instruction. 
 */

function f() {
  return 2;
}

return 1 + f();

