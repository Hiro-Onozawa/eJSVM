# indir : 入力データファイル格納ディレクトリ
# outdir : 出力グラフファイル格納ディレクトリ
# algorithm : GCアルゴリズム
# prop : 要求量 (request) / オブジェクトヘッダ (header)
# type : 確保量 (size) [byte] / 比率 (rate)

file=indir."/".algorithm."_".prop."_".type.".txt"
fileout=outdir."/".algorithm."_".prop."_".type.".eps"

set xlabel "benchmark"
if (type eq "size") set ylabel prop." size [byte]"
if (type eq "rate") set ylabel prop." size rate"
set title prop." ".type

# write to eps
set terminal eps
set output fileout

# plot
set style histogram rowstacked
set style fill solid border lc rgb "black"
set xtics rotate by -90
plot file using 2:xtic(1)  with histogram title "STRING",\
     file using 3          with histogram title "FLONUM",\
     file using 4          with histogram title "OBJECT",\
     file using 5          with histogram title "ARRAY",\
     file using 6          with histogram title "FUNCTION",\
     file using 7          with histogram title "BUILTIN",\
     file using 8          with histogram title "ITERATOR",\
     file using 9          with histogram title "BOX STRING",\
     file using 10         with histogram title "BOX NUMBER",\
     file using 11         with histogram title "BOX BOOLEAN",\
     file using 12         with histogram title "PROP",\
     file using 13         with histogram title "ARRAY DATA",\
     file using 14         with histogram title "FUNCTION FRAME",\
     file using 15         with histogram title "HASH BODY",\
     file using 16         with histogram title "STR CONS",\
     file using 17         with histogram title "STACK",\
     file using 18         with histogram title "HIDDEN CLASS"

