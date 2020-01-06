# indir1 : 入力データファイル格納ディレクトリ1
# indir2 : 入力データファイル格納ディレクトリ2
# outdir : 出力グラフファイル格納ディレクトリ
# benchname : ベンチマーク名
# threashold : スレッショルド
# basebit : 実行環境のビット幅
# param : 利用するカラムのインデックス (1 実行時間, 2 総GC時間, 3 非GC時間, 4 最大GC時間, 5 平均GC時間, 6 GC回数)
# lang : jp / en <=> 日本語 / 英語

font_style="Arial,18"

array files[2] = [\
    indir1."/mark_sweep_".threashold."_".benchname.".txt",\
    indir2."/mark_sweep_".threashold."_".benchname.".txt" \
]
fileout=outdir."/".threashold."_".benchname.".eps"
tableout=outdir."/".threashold."_".benchname."_values.txt"

if (lang eq "en") {
    array ylabels[6] = [\
        "total CPU time [msec]", "total GC time [msec]", "non GC time [msec]",\
        "max GC time [msec]", "avr GC time [msec]", "GC count"\
    ]
    array linelabels[4] = [\
        "for 32bit implement", "for 64bit implement"\
    ]
    linewidth=0
} else {
    array ylabels[6] = [\
        "実行時間 [msec]", "総 GC 時間 [msec]", "非 GC 時間 [msec]",\
        "最大 GC 時間 [msec]", "平均 GC 時間 [msec]", "GC 回数"\
    ]
    array linelabels[2] = [\
        "32ビット向け実装", "64ビット向け実装"\
    ]
    linewidth=-8
}

if (lang eq "en") {
    set xlabel "heap size [KiB]" font font_style
    set ylabel ylabels[param] font font_style offset -2.5,0
#    set title "[".basebit."bit] benchmark : ".benchname.", threashold : ".threashold font font_style
} else {
    set xlabel "ヒープサイズ [KiB]" font font_style
    set ylabel ylabels[param] font font_style offset -2.5,0
#    set title "[".basebit."ビット] ベンチマーク : ".benchname.", スレッショルド : ".threashold font font_style
}
xmax=10
set xtics ("480" 1, "640" 2, "960" 3, "1280" 4, "1920" 5, "2560" 6, "3840" 7, "5120" 8, "7680" 9, "10240" 10) font font_style
array sizes[xmax] = ["480", "640", "960", "1280", "1920", "2560", "3840", "5120", "7680", "10240"]
set ytics font font_style

# write to eps
set terminal eps
set output fileout
set lmargin 13
set rmargin 6

# plot
print "plot to ".threashold."_".benchname

ymax=0;
if (param == 1) {
    ythreashold=100000
    set key right top font font_style
} else { if (param == 2) {
    ythreashold=10000
    set key right top font font_style
} else { if (param == 3) {
    set key right top font font_style
    ythreashold=10000
} else { if (param == 4) {
    set key left top font font_style width linewidth
    ythreashold=10000
} else { if (param == 5) {
    set key left top font font_style width linewidth
    ythreashold=10000
} else {
    set key right top font font_style
    ythreashold=6000
} } } } }

array Plots[xmax*2]
do for [j=1:2] {
    STATS_blocks=0
    stats files[j] nooutput
    if (STATS_blocks > 0) {
        do for [i=1:xmax]{
            STATS_mean=-1
            stats files[j] using 2 index i-1 nooutput
            if (STATS_mean >= 0) {
                if (!(param == 2 || param == 4 || param == 5) || STATS_mean > 0) { Plots[(j-1)*xmax+i]=STATS_mean }
                if (STATS_mean > ymax && STATS_mean < ythreashold) { ymax = STATS_mean }
            }
        }
    }
}

set autoscale xfix
set clip one
set clip two
set yrange [*:ymax*1.15]
set xrange [1:xmax]

# using (x座標):データの列:(箱の幅(0のときデフォルト値)):データ区分の列
plot Plots every ::0+xmax*0::xmax*1-1 using ($1-xmax*0):2 with linespoints pt 1 ps 0.75 title linelabels[1],\
     Plots every ::0+xmax*1::xmax*2-1 using ($1-xmax*1):2 with linespoints pt 2 ps 0.75 title linelabels[2]


# write table to txt
set print tableout
print "\\hline"
if (lang eq "en") {
    print "Heap Size [KiB] & for 32bit implement [ms] & for 64bit implement [ms] \\\\ \\hline \\hline"
} else {
    print "ヒープサイズ [KiB] & 32ビット向け実装 [ms] & 64ビット向け実装 [ms] \\\\ \\hline \\hline"
}
do for [i=1:xmax]{
    print sizes[i], " & ", Plots[i+xmax*0], " & ", Plots[i+xmax*1], " \\\\"
}
print "\\hline"
set print "-"