sh scripts/generate_data.sh
sh scripts/build_linux.sh
./build/bin/temp_monitor --http 8080

.\scripts\build_windows.cmd
.\build\bin\generate_test_data.exe build/bin/temperature.db
.\build\bin\temp_monitor.exe --http 8080