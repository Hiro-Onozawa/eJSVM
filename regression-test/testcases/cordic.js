/*
 Copyright (C) Rich Moore.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

/////. Start CORDIC

var AG_CONST = 0.6072529350;

var FIXED = function(X)
{
  return X * 65536.0;
}

var FLOAT = function(X){
  return X / 65536.0;
}

var DEG2RAD = function(X)
{
  return 0.017453 * (X);
}

var Angles = new Array(12);
Angles[0] = FIXED(45.0);
Angles[1] = FIXED(26.565);
Angles[2] = FIXED(14.0362);
Angles[3] = FIXED(7.12502);
Angles[4] = FIXED(3.57633);
Angles[5] = FIXED(1.78991);
Angles[6] = FIXED(0.895174);
Angles[7] = FIXED(0.447614);
Angles[8] = FIXED(0.223811);
Angles[9] = FIXED(0.111906);
Angles[10] = FIXED(0.055953);
Angles[11] = FIXED(0.027977);


var cordicsincos = function(){
    var X;
    var Y;
    var TargetAngle;
    var CurrAngle;
    var Step;
    var NewX;
 
    X = FIXED(AG_CONST);         /* AG_CONST * cos(0) */
    Y = 0;                       /* AG_CONST * sin(0) */

    TargetAngle = FIXED(28.027);
    CurrAngle = 0;
    for (Step = 0; Step < 12; Step++) {
        if (TargetAngle > CurrAngle) {
            NewX = X - (Y >> Step);
            Y = (X >> Step) + Y;
            X = NewX;
            CurrAngle += Angles[Step];
        } else {
            NewX = X + (Y >> Step);
            Y = -(X >> Step) + Y;
            X = NewX;
            CurrAngle -= Angles[Step];
        }
    }
}

///// End CORDIC

var cordic = function( runs ) {
	var i;
  for (i = 0 ; i < runs ; i++ ) {
      cordicsincos();
  }

}

cordic(25000);
