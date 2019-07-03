// 3D Cube Rotation
// http://www.speich.net/computer/moztesting/3d.htm
// Created by Simon Speich

var Q = new Array();
var MTrans = new Array();  // transformation matrix
var MQube = new Array();  // position information of qube
var I = new Array();      // entity matrix
var Origin = new Object();
var Testing = new Object();
var LoopTimer;

var DisplArea = new Object();
DisplArea.Width = 300;
DisplArea.Height = 300;

var DrawLine = function (From, To) {
  var x1 = From.V[0];
  var x2 = To.V[0];
  var y1 = From.V[1];
  var y2 = To.V[1];
  var dx = Math.abs(x2 - x1);
  var dy = Math.abs(y2 - y1);
  var x = x1;
  var y = y1;
  var IncX1;
	var IncY1;
  var IncX2;
	var IncY2;  
  var Den;
  var Num;
  var NumAdd;
  var NumPix;
	var i;
  if (x2 >= x1) {  IncX1 = 1; IncX2 = 1;  }
  else { IncX1 = -1; IncX2 = -1; }
  if (y2 >= y1)  {  IncY1 = 1; IncY2 = 1; }
  else { IncY1 = -1; IncY2 = -1; }
  if (dx >= dy) {
    IncX1 = 0;
    IncY2 = 0;
    Den = dx;
    Num = dx / 2;
    NumAdd = dy;
    NumPix = dx;
  }
  else {
    IncX2 = 0;
    IncY1 = 0;
    Den = dy;
    Num = dy / 2;
    NumAdd = dx;
    NumPix = dy;
  }

  NumPix = Math.round(Q.LastPx + NumPix);

  i = Q.LastPx;
  for (; i < NumPix; i++) {
    Num += NumAdd;
    if (Num >= Den) {
      Num -= Den;
      x += IncX1;
      y += IncY1;
    }
    x += IncX2;
    y += IncY2;
  }
  Q.LastPx = NumPix;
}

var CalcCross = function (V0, V1) {
  var Cross = new Array();
  Cross[0] = V0[1]*V1[2] - V0[2]*V1[1];
  Cross[1] = V0[2]*V1[0] - V0[0]*V1[2];
  Cross[2] = V0[0]*V1[1] - V0[1]*V1[0];
  return Cross;
}

var CalcNormal = function (V0, V1, V2) {
  var A = new Array();   var B = new Array(); 
  var Length; 
  var i;
  for (i = 0; i < 3; i++) {
    A[i] = V0[i] - V1[i];
    B[i] = V2[i] - V1[i];
  }
  A = CalcCross(A, B);
  Length = Math.sqrt(A[0]*A[0] + A[1]*A[1] + A[2]*A[2]); 
  for (i = 0; i < 3; i++) A[i] = A[i] / Length;
  A[3] = 1;
  return A;
}

var CreateP = function(X,Y,Z) {
  this.V = [,,,];
  this.V[0] = X;
  this.V[1] = Y;
  this.V[2] = Z;
  this.V[3] = 1;
}

// multiplies two matrices
var MMulti = function(M1, M2) {
  var i = 0;
  var j = 0;
  var M = new Array(4);
  for (i = 0; i < 4; i++) {
    M[i] = new Array();
  }
  for (i = 0; i < 4; i++) {
    j = 0;
    for (; j < 4; j++) M[i][j] = M1[i][0] * M2[0][j] + M1[i][1] * M2[1][j] + M1[i][2] * M2[2][j] + M1[i][3] * M2[3][j];
  }
  return M;
}

//multiplies matrix with vector
var VMulti = function(M, V) {
  var Vect = new Array();
  var i = 0;
  for (;i < 4; i++) Vect[i] = M[i][0] * V[0] + M[i][1] * V[1] + M[i][2] * V[2] + M[i][3] * V[3];
  return Vect;
}

var VMulti2 = function(M, V) {
  var Vect = new Array();
  var i = 0;
  for (;i < 3; i++) Vect[i] = M[i][0] * V[0] + M[i][1] * V[1] + M[i][2] * V[2];
  return Vect;
}

// add to matrices
var MAdd = function(M1, M2) {
  var M = new Array(4);
  var i = 0;
  var j = 0;
  for (i = 0; i < 4; i++) {
    M[i] = new Array();
  }
  for (i = 0; i < 4; i++) {
    j = 0;
    for (; j < 4; j++) M[i][j] = M1[i][j] + M2[i][j];
  }
  return M;
}


//2011/09/06ここまで
var Translate = function(M, Dx, Dy, Dz) {
  var T = new Array(4);
  var i;
  for ( i = 0; i < 4; i++) {
    T[i] = new Array(4);
  }
  T[0][0] = 1;
  T[0][1] = 0;
  T[0][2] = 0;
  T[0][3] = Dx;
  T[1][0] = 0;
  T[1][1] = 1;
  T[1][2] = 0;
  T[1][3] = Dy;
  T[2][0] = 0;
  T[2][1] = 0;
  T[2][2] = 1;
  T[2][3] = Dz;
  T[3][0] = 0;
  T[3][1] = 0;
  T[3][2] = 0;
  T[3][3] = 1;
  return MMulti(T, M);
}

var RotateX = function(M, Phi) {
  var a = Phi;
  var Cos; 
  var Sin; 
  var R;
  var i;
  a *= Math.PI / 180;
  Cos = Math.cos(a);
  Sin = Math.sin(a);
  R = new Array(4);
  for ( i = 0; i < 4; i++) {
    R[i] = new Array(4);
  }
  R[0][0] = 1;
  R[0][1] = 0;
  R[0][2] = 0;
  R[0][3] = 0;
  R[1][0] = 0;
  R[1][1] = Cos;
  R[1][2] = -Sin;
  R[1][3] = 0;
  R[2][0] = 0;
  R[2][1] = 0;
  R[2][2] = Sin;
  R[2][3] = Cos;
  R[3][0] = 0;
  R[3][1] = 0;
  R[3][2] = 0;
  R[3][3] = 1;
  
  return MMulti(R, M);
}

var RotateY = function(M, Phi) {

  var a = Phi;
  var Cos; 
  var Sin; 
  var R;
  var i;
  a *= Math.PI / 180;
  Cos = Math.cos(a);
  Sin = Math.sin(a);
  R = new Array(4);
  for ( i = 0; i < 4; i++) {
    R[i] = new Array(4);
  }
  R[0][0] = Cos;
  R[0][1] = 0;
  R[0][2] = Sin;
  R[0][3] = 0;
  R[1][0] = 0;
  R[1][1] = 1;
  R[1][2] = 0;
  R[1][3] = 0;
  R[2][0] = -Sin;
  R[2][1] = 0;
  R[2][2] = Cos;
  R[2][3] = 0;
  R[3][0] = 0;
  R[3][1] = 0;
  R[3][2] = 0;
  R[3][3] = 1;
  
  return MMulti(R, M);
}

var RotateZ = function(M, Phi) {
  var a = Phi;
  var Cos; 
  var Sin; 
  var R;
  var i;
  a *= Math.PI / 180;
  Cos = Math.cos(a);
  Sin = Math.sin(a);
  R = new Array(4);
  for ( i = 0; i < 4; i++) {
    R[i] = new Array(4);
  }
  R[0][0] = Cos;
  R[0][1] = -Sin;
  R[0][2] = 0;
  R[0][3] = 0;
  R[1][0] = Sin;
  R[1][1] = Cos;
  R[1][2] = 0;
  R[1][3] = 0;
  R[2][0] = 0;
  R[2][1] = 0;
  R[2][2] = 1;
  R[2][3] = 0;
  R[3][0] = 0;
  R[3][1] = 0;
  R[3][2] = 0;
  R[3][3] = 1;
  return MMulti(R, M);
}

var DrawQube = function() {
  // calc current normals
  var i = 5;
  var CurN = new Array(i);
  Q.LastPx = 0;
  for (; i > -1; i--) CurN[i] = VMulti2(MQube, Q.Normal[i]);
  if (CurN[0][2] < 0) {
    if (Q.Line[0]) {} else{ DrawLine(Q[0], Q[1]); Q.Line[0] = true; };
    if (Q.Line[1]) {} else{ DrawLine(Q[1], Q[2]); Q.Line[1] = true; };
    if (Q.Line[2]) {} else{ DrawLine(Q[2], Q[3]); Q.Line[2] = true; };
    if (Q.Line[3]) {} else{ DrawLine(Q[3], Q[0]); Q.Line[3] = true; };
  }
  if (CurN[1][2] < 0) {
    if (Q.Line[2]) {} else{ DrawLine(Q[3], Q[2]); Q.Line[2] = true; };
    if (Q.Line[9]) {} else{ DrawLine(Q[2], Q[6]); Q.Line[9] = true; };
    if (Q.Line[6]) {} else{ DrawLine(Q[6], Q[7]); Q.Line[6] = true; };
    if (Q.Line[10]) {} else{ DrawLine(Q[7], Q[3]); Q.Line[10] = true; };
  }
  if (CurN[2][2] < 0) {
    if (Q.Line[4]) {} else{ DrawLine(Q[4], Q[5]); Q.Line[4] = true; };
    if (Q.Line[5]) {} else{ DrawLine(Q[5], Q[6]); Q.Line[5] = true; };
    if (Q.Line[6]) {} else{ DrawLine(Q[6], Q[7]); Q.Line[6] = true; };
    if (Q.Line[7]) {} else{ DrawLine(Q[7], Q[4]); Q.Line[7] = true; };
  }
  if (CurN[3][2] < 0) {
    if (Q.Line[4]) {} else{ DrawLine(Q[4], Q[5]); Q.Line[4] = true; };
    if (Q.Line[8]) {} else{ DrawLine(Q[5], Q[1]); Q.Line[8] = true; };
    if (Q.Line[0]) {} else{ DrawLine(Q[1], Q[0]); Q.Line[0] = true; };
    if (Q.Line[11]) {} else{ DrawLine(Q[0], Q[4]); Q.Line[11] = true; };
  }
  if (CurN[4][2] < 0) {
    if (Q.Line[11]) {} else{ DrawLine(Q[4], Q[0]); Q.Line[11] = true; };
    if (Q.Line[3]) {} else{ DrawLine(Q[0], Q[3]); Q.Line[3] = true; };
    if (Q.Line[10]) {} else{ DrawLine(Q[3], Q[7]); Q.Line[10] = true; };
    if (Q.Line[7]) {} else{ DrawLine(Q[7], Q[4]); Q.Line[7] = true; };
  }
  if (CurN[5][2] < 0) {
    if (Q.Line[8]) {} else{ DrawLine(Q[1], Q[5]); Q.Line[8] = true; };
    if (Q.Line[5]) {} else{ DrawLine(Q[5], Q[6]); Q.Line[5] = true; };
    if (Q.Line[9]) {} else{ DrawLine(Q[6], Q[2]); Q.Line[9] = true; };
    if (Q.Line[1]) {} else{ DrawLine(Q[2], Q[1]); Q.Line[1] = true; };
  }
    Q.Line = new Array(12);
  for (i = 0; i < 12; i++) {
    Q.Line[i] = false;
  }
  Q.LastPx = 0;
}

var Loop = function() {
  var TestingStr;
  var i;
  if (Testing.LoopCount > Testing.LoopMax) return;
  TestingStr = String(Testing.LoopCount);
  while (TestingStr.length < 3) TestingStr = "0" + TestingStr;
  MTrans = Translate(I, -Q[8].V[0], -Q[8].V[1], -Q[8].V[2]);
  MTrans = RotateX(MTrans, 1);
  MTrans = RotateY(MTrans, 3);
  MTrans = RotateZ(MTrans, 5);
  MTrans = Translate(MTrans, Q[8].V[0], Q[8].V[1], Q[8].V[2]);
  MQube = MMulti(MTrans, MQube);
	i = 8;
  for (; i > -1; i--) {
    Q[i].V = VMulti(MTrans, Q[i].V);
  }
  DrawQube();
  Testing.LoopCount++;
  Loop();
}

var Init = function(CubeSize) {
  // init/reset vars
  var i,j;
  Origin.V = new Array(4);
  Origin.V[0] = 150;
  Origin.V[1] = 150;
  Origin.V[2] = 20;
  Origin.V[3] = 1;
  Testing.LoopCount = 0;
  Testing.LoopMax = 50;
  Testing.TimeMax = 0;
  Testing.TimeAvg = 0;
  Testing.TimeMin = 0;
  Testing.TimeTemp = 0;
  Testing.TimeTotal = 0;
  Testing.Init = false;

  // transformation matrix
  MTrans = new Array(4);
  for ( i = 0; i < 4; i++) {
    MTrans[i] = new Array(4);
    for  (j = 0; j < 4; j++) {
      MTrans[i][j] = 0;
    }
    MTrans[i][i] = 1;
  }
  // position information of qube
  MQube = new Array(4);
  for ( i = 0; i < 4; i++) {
    MQube[i] = new Array(4);
    for  (j = 0; j < 4; j++) {
      MQube[i][j] = 0;
    }
    MQube[i][i] = 1;
  }
  
  // entity matrix
  I = new Array(4);
  for ( i = 0; i < 4; i++) {
    I[i] = new Array(4);
    for  (j = 0; j < 4; j++) {
      I[i][j] = 0;
    }
    I[i][i] = 1;
  }
  
  // create qube
  Q[0] = new CreateP(-CubeSize,-CubeSize, CubeSize);
  Q[1] = new CreateP(-CubeSize, CubeSize, CubeSize);
  Q[2] = new CreateP( CubeSize, CubeSize, CubeSize);
  Q[3] = new CreateP( CubeSize,-CubeSize, CubeSize);
  Q[4] = new CreateP(-CubeSize,-CubeSize,-CubeSize);
  Q[5] = new CreateP(-CubeSize, CubeSize,-CubeSize);
  Q[6] = new CreateP( CubeSize, CubeSize,-CubeSize);
  Q[7] = new CreateP( CubeSize,-CubeSize,-CubeSize);
  
  // center of gravity
  Q[8] = new CreateP(0, 0, 0);
  
  // anti-clockwise edge check
  Q.Edge = new Array(6);
  for ( i = 0; i < 6; i++) {
    Q.Edge[i] = new Array(3);
  }
  Q.Edge[0][0] = 0;
  Q.Edge[0][1] = 1;
  Q.Edge[0][2] = 2;
  Q.Edge[1][0] = 3;
  Q.Edge[1][1] = 2;
  Q.Edge[1][2] = 6;
  Q.Edge[2][0] = 7;
  Q.Edge[2][1] = 6;
  Q.Edge[2][2] = 5;
  Q.Edge[3][0] = 4;
  Q.Edge[3][1] = 5;
  Q.Edge[3][2] = 1;
  Q.Edge[4][0] = 4;
  Q.Edge[4][1] = 0;
  Q.Edge[4][2] = 3;
  Q.Edge[5][0] = 1;
  Q.Edge[5][1] = 5;
  Q.Edge[5][2] = 6;
  
  // calculate squad normals
  Q.Normal = new Array();
  for (var i = 0; i < Q.Edge.length; i++) Q.Normal[i] = CalcNormal(Q[Q.Edge[i][0]].V, Q[Q.Edge[i][1]].V, Q[Q.Edge[i][2]].V);
  
  // line drawn ?
  Q.Line = new Array(12);
  for (i = 0; i < 12; i++) {
    Q.Line[i] = false;
  }
  
  // create line pixels
  Q.NumPx = 9 * 2 * CubeSize;
  for (var i = 0; i < Q.NumPx; i++) CreateP(0,0,0);
  MTrans = Translate(MTrans, Origin.V[0], Origin.V[1], Origin.V[2]);
  MQube = MMulti(MTrans, MQube);

  var i = 0;
  for (; i < 9; i++) {
    Q[i].V = VMulti(MTrans, Q[i].V);
  }
  DrawQube();
  Testing.Init = true;
  Loop();
}

for ( var i = 20; i <= 160; i *= 2 ) {
  Init(i);
}

Q = null;
MTrans = null;
MQube = null;
I = null;
Origin = null;
Testing = null;
LoopTime = null;
DisplArea = null;
