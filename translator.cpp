#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <regex>
#include <boost/filesystem.hpp>

using add_t = unsigned;

class Parser{
public:
	enum enumType {
		C_ARITHEMIC,C_PUSH,C_POP,C_LABEL,C_GOTO,C_IF,C_FUNCTION,C_RETURN,C_CALL
	};

	Parser(const std::string& filename):ifs(filename),line_count(){
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
	std::string arg1_val;
	add_t arg2_val;
	unsigned long long line_count;

	bool parse_core(){
		std::string buf;

		READ_VAL:
		if(!std::getline(ifs,buf)) return false;
		++line_count;

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

	void regex_core(const std::string& str){
		#define SYMBOL_MATCH "([a-zA-Z_.$:][a-zA-Z\\d_.$:]*)"
		#define DEF_REGEX(name,str) static const std::regex name("^\\s*" str "\\s*$")

		DEF_REGEX(arith_match,"(add|sub|neg|eq|gt|lt|and|or|not)");
		DEF_REGEX(stack_match,"(push|pop)\\s+(argument|local|static|constant|this|that|pointer|temp)\\s+(\\d)\\s+");
		DEF_REGEX(flow_match,"(label|goto|if-goto)\\s+" SYMBOL_MATCH);
		DEF_REGEX(func_call_match,"(function|call)\\s+" SYMBOL_MATCH "\\s+\\d");
		DEF_REGEX(return_match,"return");

		#undef SYMBOL_MATCH
		#undef DEF_REGEX

		std::smatch match;
		if(std::regex_match(str,match,arith_match)){
			type = C_ARITHEMIC;
			arg1_val = match[1].str();
		}
		else if(std::regex_match(str,match,stack_match)){
			if(match[1].str() == "push") type = C_PUSH;
			else type = C_POP;
			arg1_val = match[2].str();
			arg2_val = std::stoul(match[3].str());
		}
		else if(std::regex_match(str,match,flow_match)){
			if(match[1].str() == "label") type = C_LABEL;
			else if(match[1].str() == "goto") type = C_GOTO;
			else type = C_IF;
			arg1_val = match[2].str();
		}
		else if(std::regex_match(str,match,func_call_match)){
			if(match[1].str() == "func") type = C_FUNCTION;
			else type = C_CALL;
			arg1_val = match[2].str();
			arg2_val = std::stoul(match[3].str());
		}
		else{
			throw std::runtime_error("Unknown instruction \""+str+"\" in line " + std::to_string(line_count) + "\n");
		}
	}
};

class CodeWriter{
public:
	CodeWriter(const std::string& filename):ofs(filename){
		if(!ofs.is_open()) throw std::runtime_error("Could not open output file\n");
	}
	void setFileName(const std::string& filename){
		; // currently no-op
	}
	void writeArithemic(const std::string& command){
		#define COMP(com) "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@TRUE\nD;" com "\n@SP\nA=M-1\nM=-1\n@END\n0;JMP\n(TRUE)\nA=M-1\nM=0\n(END)\n"
		#define BIN_AR(op) "@SP\nAM=M-1\nD=M\nA=A-1\n" op "\n"
		#define UNA_AR(op) "@SP\nA=M-1\n" op "\n"
		const static std::map<std::string,std::function<void()>> commandTable = {
			{"add",[&]{ofs << BIN_AR("M=D+M");}},
			{"sub",[&]{ofs << BIN_AR("M=M-D");}},
			{"neg",[&]{ofs << UNA_AR("M=-M");}},
			{"eq", [&]{ofs << COMP("JEQ");}},
			{"gt", [&]{ofs << COMP("JGT");}},
			{"lt", [&]{ofs << COMP("JLT");}},
			{"and",[&]{ofs << BIN_AR("M=D&M");}},
			{"or", [&]{ofs << BIN_AR("M=D|M");}},
			{"not",[&]{ofs << UNA_AR("M=!M");}},
		};
		#undef COMP
		#undef BIN_AR
		#undef UNA_AR
		commandTable.at(command)();
	}
	void writePushPop(Parser::enumType command, std::string segment, add_t index){
		switch(command){
			case Parser::C_PUSH:{

			}
			case Parser::C_POP:{

			}
		}
	}
	void writeLabel(const std::string& label);
	void writeGoto(const std::string& label);
	void writeIf(const std::string& label);
	void writeFunction(const std::string& function_name, add_t num_locals);
	void writeReturn();
	void writeCall(const std::string& function_name, add_t num_args);
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
			switch(p.commandType()){
				case Parser::C_ARITHEMIC:{
					cw.writeArithemic(p.arg1());
					break;
				}
				case Parser::C_PUSH:{
					cw.writePushPop(Parser::C_PUSH,p.arg1(),p.arg2());
					break;
				}
				case Parser::C_POP:{
					cw.writePushPop(Parser::C_POP,p.arg1(),p.arg2());
					break;
				}
				case Parser::C_LABEL:{
					cw.writeLabel(p.arg1());
					break;
				}
				case Parser::C_GOTO:{
					cw.writeLabel(p.arg1());
					break;
				}
				case Parser::C_IF:{
					cw.writeIf(p.arg1());
					break;
				}
				case Parser::C_FUNCTION:{
					cw.writeFunction(p.arg1(),p.arg2());
					break;
				}
				case Parser::C_RETURN:{
					cw.writeReturn();
					break;
				}
				case Parser::C_CALL:{
					cw.writeCall(p.arg1(),p.arg2());
					break;
				}
			}
		}
	}
}
