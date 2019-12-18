# indir : 入力データファイル格納ディレクトリ
# outdir : 出力グラフファイル格納ディレクトリ
# benchname : ベンチマーク名
# threashold : スレッショルド
# basebit : 実行環境のビット幅
# ylabel_title : y軸の表記
# label_max : x軸のメモリの表記の最大値
# label_min : x軸のメモリの表記の最小値

file1=indir."/mark_sweep_".threashold."_".benchname.".txt"
file2=indir."/mark_compact_".threashold."_".benchname.".txt"
file3=indir."/threaded_compact_".threashold."_".benchname.".txt"
file4=indir."/copy_".threashold."_".benchname.".txt"
fileout=outdir."/".threashold."_".benchname.".eps"

set xlabel "heap size [byte]"
set ylabel ylabel_title
set title "[".basebit."bit] benchmark : ".benchname.", threashold : ".threashold
if (label_max==10485760 && label_min==1310720){
    set xtics ("10485760" 1, "7864320" 2, "5242880" 3, "3932160" 4, "2621440" 5, "1966080" 6, "1310720" 7) rotate by -25
    xmax=7
} else {
    if (label_max==3932160 && label_min==491520){
        set xtics ("3932160" 1, "2621440" 2, "1966080" 3, "1310720" 4, "983040" 5, "655360" 6, "491520" 7) rotate by -25
        xmax=7
    } else {
        set xtics ("10485760" 1, "7864320" 2, "5242880" 3, "3932160" 4, "2621440" 5, "1966080" 6, "1310720" 7, "983040" 8, "655360" 9, "491520" 10) rotate by -25
        xmax=10
    }
}

# write to eps
set terminal eps
set output fileout

# plot
print "plot to ".threashold."_".benchname

ymax=0;
ythreashold=10000

STATS_blocks=0
stats file1 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array A[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file1 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            A[i]=STATS_median
            if (A[i] > ymax && A[i] < ythreashold) { ymax = A[i] }
        }
    }
} else {
    array A[1]
}
STATS_blocks=0
stats file2 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array B[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file2 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            B[i]=STATS_median
            if (B[i] > ymax && B[i] < ythreashold) { ymax = B[i] }
        }
    }
} else {
    array B[1]
}
STATS_blocks=0
stats file3 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array C[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file3 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            C[i]=STATS_median
            if (C[i] > ymax && C[i] < ythreashold) { ymax = C[i] }
        }
    }
} else {
    array C[1]
}
STATS_blocks=0
stats file4 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array D[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file4 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            D[i]=STATS_median
            if (D[i] > ymax && D[i] < ythreashold) { ymax = D[i] }
        }
    }
} else {
    array D[1]
}

set autoscale xfix
set clip one
set clip two
set yrange [*:ymax*1.25]
set xrange [1:xmax]

# using (x座標):データの列:(箱の幅(0のときデフォルト値)):データ区分の列
plot A using 1:2 with linespoints pt 1 ps 0.75 title "mark sweep",\
     B using 1:2 with linespoints pt 2 ps 0.75 title "mark compact",\
     C using 1:2 with linespoints pt 3 ps 0.75 title "threaded compact",\
     D using 1:2 with linespoints pt 4 ps 0.75 title "copy"
