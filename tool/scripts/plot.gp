# indir : 入力データファイル格納ディレクトリ
# outdir : 出力グラフファイル格納ディレクトリ
# benchname : ベンチマーク名
# threashold : スレッショルド
# label_max : x軸のメモリの表記の最大値
# label_min : x軸のメモリの表記の最小値

file1=indir."/mark_sweep_".threashold."_".benchname.".txt"
file2=indir."/mark_compact_".threashold."_".benchname.".txt"
file3=indir."/threaded_compact_".threashold."_".benchname.".txt"
file4=indir."/copy_".threashold."_".benchname.".txt"
fileout=outdir."/".threashold."_".benchname.".eps"

set xlabel "heap size [byte]"
set ylabel "GC time [msec]"
set title "benchmark : ".benchname.", threashold : ".threashold
if (label_max==10485760 && label_min==1310720){
    set xtics ("10485760" 1, "7864320" 2, "5242880" 3, "3932160" 4, "2621440" 5, "1966080" 6, "1310720" 7) rotate by -25
} else {
    if (label_max==3932160 && label_min==491520){
        set xtics ("3932160" 1, "2621440" 2, "1966080" 3, "1310720" 4, "983040" 5, "655360" 6, "491520" 7) rotate by -25
    } else {
        set xtics ("10485760" 1, "7864320" 2, "5242880" 3, "3932160" 4, "2621440" 5, "1966080" 6, "1310720" 7, "983040" 8, "655360" 9, "491520" 10) rotate by -25
    }
}

# write to eps
set terminal eps
set output fileout

# plot
set autoscale xfix

print "plot to ".threashold."_".benchname

STATS_blocks=0
stats file1 nooutput; if (STATS_blocks > 0) { array A[STATS_blocks] } else { array A[1] }
do for [i=0:STATS_blocks-2]{ stats file1 using 2 index i nooutput; A[i+1]=STATS_median };
STATS_blocks=0
stats file2 nooutput; if (STATS_blocks > 0) { array B[STATS_blocks] } else { array B[1] }
do for [i=0:STATS_blocks-2]{ stats file2 using 2 index i nooutput; B[i+1]=STATS_median };
STATS_blocks=0
stats file3 nooutput; if (STATS_blocks > 0) { array C[STATS_blocks] } else { array C[1] }
do for [i=0:STATS_blocks-2]{ stats file3 using 2 index i nooutput; C[i+1]=STATS_median };
STATS_blocks=0
stats file4 nooutput; if (STATS_blocks > 0) { array D[STATS_blocks] } else { array D[1] }
do for [i=0:STATS_blocks-2]{ stats file4 using 2 index i nooutput; D[i+1]=STATS_median };

# using (x座標):データの列:(箱の幅(0のときデフォルト値)):データ区分の列
plot A using 1:2 with linespoints pt 1 ps 0.75 title "mark sweep",\
     B using 1:2 with linespoints pt 2 ps 0.75 title "mark compact",\
     C using 1:2 with linespoints pt 3 ps 0.75 title "threaded compact",\
     D using 1:2 with linespoints pt 4 ps 0.75 title "copy"
