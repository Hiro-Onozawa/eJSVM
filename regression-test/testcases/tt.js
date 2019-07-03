f = function(x)
{
    print(x);
    throw x;
}

    try{
	f(10);
    }
    catch(e){
	print(e);
    }finally{
    }