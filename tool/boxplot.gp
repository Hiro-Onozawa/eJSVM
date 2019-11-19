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
set title "benchmark : ".benchname.", threashold : ".threashold

# write to eps
set terminal eps
set output fileout

# plot
set autoscale xfix

set style fill solid 0.25 border -1
set style boxplot nooutliers pointtype 7
set style data boxplot
set boxwidth 1
set bars 0.5

stats file1 using 2 nooutput

plot for [i=0:STATS_blocks-1] file1 using (7*i):2 index i lt 1 title (i==0 ? 'mark sweep' : ''),\
     for [i=0:STATS_blocks-1] file2 using (7*i+1):2 index i lt 2 title (i==0 ? 'mark compact' : ''),\
     for [i=0:STATS_blocks-1] file3 using (7*i+2):2 index i lt 3 title (i==0 ? 'threaded compact' : ''),\
     for [i=0:STATS_blocks-1] file4 using (7*i+3):2 index i lt 4 title (i==0 ? 'copy' : ''),\
     for [i=0:STATS_blocks-1] file1 using (7*i+1.5):(-1):xticlabel(1) index i w l notitle

