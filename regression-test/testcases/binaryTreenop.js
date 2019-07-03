/* The Great Computer Language Shootout
   http://shootout.alioth.debian.org/
   contributed by Isaac Gouy */

var TreeNode = function (left,right,item){
   this.left = left;
   this.right = right;
   this.item = item;
}

TreeNode.prototype.itemCheck = function(){
   if (this.left==null) return this.item;
   else return this.item + this.left.itemCheck() - this.right.itemCheck();
}

var bottomUpTree = function (item,depth){
   if (depth>0){
      return new TreeNode(
          bottomUpTree(2*item-1, depth-1)
         ,bottomUpTree(2*item, depth-1)
         ,item
      );
   }
   else {
      return new TreeNode(null,null,item);
   }
}

var ret;
var n;
var minDepth;
var maxDepth;
var stretchDepth;
var check;
var longLivedTree;
var depth;
var iterations;
var i;
for ( n = 4; n <= 7; n += 1 ) {
    minDepth = 4;
    maxDepth = Math.max(minDepth + 2, n);
    stretchDepth = maxDepth + 1;
    
    check = bottomUpTree(0,stretchDepth).itemCheck();
    
    longLivedTree = bottomUpTree(0,maxDepth);
    for (depth=minDepth; depth<=maxDepth; depth+=2){
        iterations = 1 << (maxDepth - depth + minDepth);

        check = 0;
        for (i=1; i<=iterations; i++){
            check += bottomUpTree(i,depth).itemCheck();
            check += bottomUpTree(-i,depth).itemCheck();
        }
    }

    ret = longLivedTree.itemCheck();
}
