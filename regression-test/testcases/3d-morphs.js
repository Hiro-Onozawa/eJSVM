/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

var loops = 50;
var nx = 200;
var nz = nx;
var i,q,r;
q = 0;
var morph = function(a, f) {
    var PI2nx,sin,f30,i,j,p; 
		PI2nx = Math.PI * 8/nx;
		sin = Math.sin;
		f30 = -(50 * sin(f*Math.PI*2));
		i = 0;
    while(i < nz) {
				j = 0;
        while (j < nx) {
            a[3*(i*nx+j)+1]    = sin((j-1) * PI2nx ) * -f30;
            //print(a[3*(i*nx+j)+1]);
						j++;
						q++;
        }
				i++;
    }
    return a[nx/2];
}

    
var a = new Array(nx*nx*3);
i = 0;
while (i < nx*nz*3){ 
    a[i] = 0;
		i++;
		q++;
}
// for (i = 0; i < loops; ++i) {
    // morph(a, i/loops);
// }
i = 0;
r = 0;
while(i < loops){
		r = r + morph(a,1/loops);
		i++;
		q++;
}
testOutput = 0;
i = 0;
while (i < nx){
    testOutput += a[3*(i*nx+i)+1];
		i++;
		q++;
}
testOutput;
