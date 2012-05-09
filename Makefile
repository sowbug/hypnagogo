clean :
	rm -f *.b#? *.s#? *.l#? *.pro *~

zip :
	rm -f MikeTsao-Hypnagogo.zip && zip MikeTsao-Hypnagogo.zip *.dri *.xln *.ger *.gpi

cleancam :
	rm -f *.dri *.xln *.ger *.gpi
