dev:
	- rm -r build/*
	gcc src/*.c -o build/main
	echo "-------- --------"
	./build/main 8967
