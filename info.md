# Enlaces revisados

- https://github.com/hemashushu/practice-qemu-riscv64-minimal-linux?tab=readme-ov-file#31-compile-linux-kernel
- https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html
- https://stackoverflow.com/questions/11408041/how-to-debug-the-linux-kernel-with-gdb-and-qemu

# TODO

- Longitudes arbitrarias de datos (¿loop en el driver?).
- Micro-benchmarks (gettimeofday) en el codigo de usuario. DONE
- Adaptar matrix-multiply (preguntar por el codigo a Xavi).

# pistas

ver dma.c línea 37: len_left puede que se actualize incorrectamente despues de volver de añadir nuevos handles.
