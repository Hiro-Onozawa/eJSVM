var tarai = function(x, y, z){
	if(x <= y){
		return y;
	}
	return tarai(tarai(x-1, y, z), tarai(y-1, z, x), tarai(z-1, x, y));
};
tarai(1, 0, 12);
