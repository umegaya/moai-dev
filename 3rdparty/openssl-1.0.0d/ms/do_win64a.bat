
perl util\mkfiles.pl >MINFO
perl ms\uplink.pl win64a > ms\uptable.asm
ml64 -c -Foms\uptable.obj ms\uptable.asm
perl util\mk1mf.pl no-asm VC-WIN64A >ms\nt.mak
perl util\mk1mf.pl dll no-asm VC-WIN64A >ms\ntdll.mak

perl util\mkdef.pl 32 libeay > ms\libeay32.def
perl util\mkdef.pl 32 ssleay > ms\ssleay32.def
