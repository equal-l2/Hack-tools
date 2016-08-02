#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include <boost/filesystem.hpp>

using add_t = unsigned;

class Parser{
public:
	enum enumType {
		C_ARITHEMIC,C_PUSH,C_POP,C_LABEL,C_GOTO,C_IF,C_FUNCTION,C_RETURN,C_CALL
	};

	Parser(std::string filename):ifs(filename){
		if(!ifs.is_open()) throw std::runtime_error("File is not found\n");
	}
	bool hasMoreCommands(){
		return parse_core();
	}
	void advance(){
		; // intentionally no-op
	}
	enumType commandType(){
		return type;
	}
	std::string arg1();
	add_t arg2();
private:
	std::ifstream ifs;
	enumType type;

	bool parse_core(){
		std::string buf;

		READ_VAL:
		if(!std::getline(ifs,buf)) return false;

		{
			/* erase comment */
			auto n = buf.find("//");
			if(n != buf.npos) buf.erase(n);
		}

		if(std::all_of(buf.begin(),buf.end(),isspace)) goto READ_VAL;

		try{
			regex_core(buf);
		}
		catch(std::runtime_error& e){
			std::cout << e.what() << std::endl;
			throw e;
		}
		return true;
	}

	void regex_core(std::string str);
};

class CodeWriter{
public:
	CodeWriter(std::string filename):ofs(filename){
		if(!ofs.is_open()) throw std::runtime_error("Could not open output file\n");
	}
	void setFileName(std::string filename);
	void writeArithemic(std::string command);
	void writePushPop(Parser::enumType command, std::string segment, add_t index);
	void close(){
		ofs.close();
	}
private:
	std::ofstream ofs;
};

int main(int argc, char** argv){

	if(argc < 2){
		std::cout << "Usage : Translator <filename or directory>\n";
		std::cout << "Input files' extensions must be \".vm\"\n";
		std::cout << "If directory is passed, all \".vm\" files will be processed into one output.\n";
		return 0;
	}

	std::vector<boost::filesystem::path> files;
	const boost::filesystem::path path_in = argv[1];
	if(boost::filesystem::is_directory(path_in)){
		for(auto&& v : boost::filesystem::directory_iterator(path_in)){
			if(v.path().extension() == ".vm") files.push_back(v.path());
		}
		if(files.empty()){
			std::cout << "No valid source files\n";
			return 0;
		}
	}
	else if(path_in.extension() != ".vm"){
		std::cout << "Bad extension : file extension must be \".vm\"\n";
		return 0;
	}
	const std::string filename_out = path_in.stem().string() + ".asm";

	CodeWriter cw(filename_out);

	for(auto&& v : files){
		Parser p(v.string());
		while(p.hasMoreCommands()){
			p.advance();
		}
	}
}
