.PHONY: app.uf2
app.uf2: 
	idf.py all
	python3 uf2/utils/uf2conv.py -f 0xbfdd4eee -b 0x0000 -c -o  app.uf2 build/sntp.bin
