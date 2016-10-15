.PHONY : all
all : Assembler Translator
Assembler : assembler.cpp
	c++ -std=c++11 $^ -o $@ -lboost_system -lboost_filesystem 
Translator : translator.cpp
	c++ -std=c++11 $^ -o $@ -lboost_system -lboost_filesystem 
