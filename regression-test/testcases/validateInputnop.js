var letters = ["a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"];
var numbers = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26];
var colors  = ["FF","CC","99","66","33","00"];

var endResult;
var loop;
loop = 1000;

var doTest = function ()
{
	var k;
	var pattern;
	var r;
	var s;
	var zipGood;
	var zip;
	var i;
	var ch;
   endResult = "";

   // make up email address
   for (k=0;k<loop;k++)
   {
      name = makeName(6);
      if(k%2 == 0){email=name+"@mac.com";}else{email=name+"(at)mac.com";}

      // validate the email address
      pattern = /^[a-zA-Z0-9\-\._]+@[a-zA-Z0-9\-_]+(\.?[a-zA-Z0-9\-_]*)\.[a-zA-Z]{2,3}$/;

      if(pattern.test(email))
      {
         r = email + "_appears_to_be_a_valid_email_address.";
         addResult(r);
      }
      else
      {
         r = email + "_does_NOT_appear_to_be_a_valid_email_address.";
         addResult(r);
      }
   }

   // make up ZIP codes
   for (s=0;s<loop;s++)
   {
      zipGood = true;
      zip = makeNumber(4);
      if(s%2 == 0){
				zip=zip+ "xyz";
			}
			else{
				zip=zip.concat("7");
			}
      // validate the zip code
      for ( i = 0; i < zip.length; i++) {
           ch = zip.charAt(i);
          if (ch < "0" || ch > "9") {
              zipGood = false;
              r = zip + "_contains_letters.";
              addResult(r);
          }
      }
      if (zipGood && zip.length>5)
      {
         zipGood = false;
         r = zip + "_is_longer_than_five_characters.";
         addResult(r);
      }
      if (zipGood)
      {
         r = zip + "_appears_to_be_a_valid_ZIP_code.";
         addResult(r);
      }
   }
}

var makeName = function(n)
{
   var tmp = "";
	 var i;
	 var l;
   for ( i=0;i<n;i++)
   {
      l = Math.floor(26*Math.random());
      tmp += letters[l];
   }
   return tmp;
}

var makeNumber = function(n)
{
   var tmp = "";
	 var i;
	 var l;
   for ( i=0;i<n;i++)
   {
      l = Math.floor(9*Math.random());
      tmp += l;//tmp.concat(l);
   }
   return tmp;
}

var addResult = function (r)
{
   endResult += "/n" + r;
}

doTest();
