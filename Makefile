beat:
	gcc -O3 src/beat.c -o beat

clean:
	rm beat

install:
	sudo mv beat /usr/local/bin/beat
	sudo cp beat.service /usr/lib/systemd/system/beat.service

uninstall:
	sudo rm /usr/local/bin/beat
	sudo rm /usr/lib/systemd/system/beat.service