CXX=/home/kwan/clang5/bin/clang++
CXXFLAGS=-std=c++17 -O3 -stdlib=libc++ -g3 -Wall -pedantic -Werror
LXXFLAGS=-Wl,-rpath=/home/kwan/clang5/lib/
OBJECTS=$(addsuffix .o, $(basename $(wildcard *.cpp)))

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tonal: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LXXFLAGS) -lc++experimental

clean:
	- rm $(OBJECTS)