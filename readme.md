# s16emu
C program that emulates a Sigma16-like CPU. It strives for minimalistic and easy
to understand code. It aims not to rely on undefined behaviour when compiled with
a C99-compliant compiler, and to fully conform to the behaviour specified in
`doc/isa.md`.

While it seem to run current Sigma16 code out there just fine, complete matching
of the JS Sigma16 emulator's behaviour is an explicitly stated **non-goal**.
When said JS emulator exhibits somewhat sane and consistent behaviour the ISA
specification and this emulator tries to match it, otherwise consistency is
preferred over 100% compatiblity.
