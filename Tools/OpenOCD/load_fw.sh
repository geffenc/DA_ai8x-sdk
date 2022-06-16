./openocd -f interface/cmsis-dap.cfg -f target/max78000.cfg -s ./scripts -c "program ../../Examples/MAX78000/GPIO/build/max78000.elf verify reset exit"
