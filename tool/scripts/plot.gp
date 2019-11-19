# indir : 入力データファイル格納ディレクトリ
# outdir : 出力グラフファイル格納ディレクトリ
# benchname : ベンチマーク名
# threashold : スレッショルド

file1=indir."/mark_sweep_".threashold."_".benchname.".txt"
file2=indir."/mark_compact_".threashold."_".benchname.".txt"
file3=indir."/threaded_compact_".threashold."_".benchname.".txt"
file4=indir."/copy_".threashold."_".benchname.".txt"
fileout=outdir."/".threashold."_".benchname.".eps"

set xlabel "heap size [byte]"
set ylabel "GC time [msec]"
set title "benchmark : ".benchname.", threshold : ".threashold

# write to eps
set terminal eps
set output fileout

# plot
#plot file.".csv" using 3:4 title "data"
plot file1 title "mark sweep", file2 title "mark compact", file3 title "threaded compact", file4 title "copy"

