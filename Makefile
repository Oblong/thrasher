preload:
	g++ -std=c++14 -fPIC -shared -o playback_preload.so playback_preload.cpp -ldl

playback:
	g++ -std=c++14 -Wfatal-errors -fmax-errors=5 -o playback playback.cpp -I. -lglfw -lGL
