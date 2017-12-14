a.out: suffix_tree.cpp
	g++ suffix_tree.cpp -std=c++11 -g

run: a.out
	./a.out
