#####################################################################
## tst_bench_qfileinfo Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qfileinfo
    SOURCES
        tst_bench_qfileinfo.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
