assembly:
	docker-compose down
	docker-compose build
	docker-compose up -d

runclient:
	.\build\client.exe .\config\client_config.json

runserver:
	docker-compose up -d

reassembly:
	docker-compose down
	docker-compose build
run:
	if exist build rmdir /s /q build
	mkdir build
	cd build && cmake .. -G "MinGW Makefiles" && cmake --build .
	docker-compose down
	docker-compose build
	docker-compose up -d
	.\build\client.exe .\config\client_config.json
comp:
	if exist build rmdir /s /q build
	mkdir build
	cd build && cmake .. -G "MinGW Makefiles" && cmake --build .