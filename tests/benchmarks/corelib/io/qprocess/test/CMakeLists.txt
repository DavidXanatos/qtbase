#####################################################################
## tst_bench_qprocess Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qprocess
    SOURCES
        ../tst_bench_qprocess.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Test
)
