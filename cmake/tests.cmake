# cmake/tests.cmake: los tests (make test), con CTest.
#
# Deja los tests games (tests/run.sh: cada juego de tests/games) y, aparte,
# transfer (tests/transfer.sh, con la etiqueta network: necesita UDP en
# 127.0.0.1). Usan el quixjs de este preset.

enable_testing()

add_test(NAME games COMMAND sh tests/run.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME transfer COMMAND sh tests/transfer.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(games transfer PROPERTIES ENVIRONMENT "QUIXJS=$<TARGET_FILE:quixjs>")
set_tests_properties(transfer PROPERTIES LABELS network)
