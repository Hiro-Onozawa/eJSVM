var A = function(i,j) {
	return 1/(( i+j)*( i+j +1)/2+ i +1);
};

var Au = function(u,v) {
	var i, j, t;
	i = 0;	
	while(i < u.length){
		t = 0;
		j = 0;
		while(j < u.length){
			t += A(i,j) * u[j];
			j++;
		}
		v[i] = t;
		i++;
	}
};

var Atu = function(u,v) {
	var i, j, t;
	i = 0;
	while(i < u.length){
		t = 0;
		j = 0;
		while(j < u.length){
			t += A(j,i) * u[j];
			j++;
		}
		v[i] = t;
		i++;
	}
};

var AtAu = function(u,v,w) {
	Au(u,w);
	Atu(w,v);
};

var spectralnorm = function(n){
	var i, u, v, w, vv, vBv;
	vv = 0;
	vBv = 0;
	u = new Array(n);
	v = new Array(n);
	w = new Array(n);
	u.length = n;
	v.length = n;
	w.length = n;
	i = 0;
	while(i < n){
		u[i] = 1; v[i] = w[i] = 0;
		i++;
	}
	i = 0;
	while(i < 10){
		AtAu (u,v,w);
		AtAu (v,u,w);
		i++;
	}
	i = 0;
	while(i < n){
		vBv += u[i]*v[i];
		vv += v[i]*v[i];
		i++;
	}
	
	// i = Math.sqrt(vBv / vv);
	// return i;
	// return vBv;
	return Math.sqrt( vBv/vv );
	// return u.length;
};


spectralnorm(700);
