# Generated from globaldata.pro.

#####################################################################
## globaldata Binary:
#####################################################################

qt_internal_add_executable(globaldata
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_globaldata.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(globaldata)
# special case end
