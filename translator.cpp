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
	std::string arg1(){
		return arg1_val;
	}
	add_t arg2(){
		return arg2_val;
	}
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
		DEF_REGEX(stack_match,"(push|pop)\\s+(argument|local|static|constant|this|that|pointer|temp)\\s+(\\d+)");
		DEF_REGEX(flow_match,"(label|goto|if-goto)\\s+" SYMBOL_MATCH);
		DEF_REGEX(func_call_match,"(function|call)\\s+" SYMBOL_MATCH "\\s+\\d+");
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
			throw std::runtime_error("Unknown instruction \""+str+"\" in line " + std::to_string(line_count));
		}
	}
};

class CodeWriter{
public:
	CodeWriter(const std::string& name):ofs(name),filename(name){
		if(!ofs.is_open()) throw std::runtime_error("Could not open output file\n");
	}
	void setFileName(const std::string& filename){
		; // currently no-op
	}
	void writeArithemic(const std::string& command){
		static unsigned long long labelnum = 0;
		std::string labelstr = std::to_string(labelnum);
		#define COMP(op) [&]{return "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@TRUE" + labelstr + "\nD;" op "\n@SP\nA=M-1\nM=0\n@END" + labelstr + "\n0;JMP\n(TRUE" + labelstr + ")\n@SP\nA=M-1\nM=-1\n(END" + labelstr + ")\n";}
		#define BIN_AR(op) []{return "@SP\nAM=M-1\nD=M\nA=A-1\n" op "\n";}
		#define UNA_AR(op) []{return "@SP\nA=M-1\n" op "\n";}
		const static std::map<std::string,std::function<std::string(void)>> commandTable = {
			{"add",BIN_AR("M=D+M")},
			{"sub",BIN_AR("M=M-D")},
			{"neg",UNA_AR("M=-M")},
			{"eq", COMP("JEQ")},
			{"gt", COMP("JGT")},
			{"lt", COMP("JLT")},
			{"and",BIN_AR("M=D&M")},
			{"or", BIN_AR("M=D|M")},
			{"not",UNA_AR("M=!M")},
		};
		#undef COMP
		#undef BIN_AR
		#undef UNA_AR
		ofs << commandTable.at(command)();
		++labelnum;
	}
	void writePushPop(Parser::enumType command, std::string segment, add_t index){
		switch(command){
			case Parser::C_PUSH:{
				if(segment == "constant"){
					ofs << "@"+std::to_string(index)+"\nD=A\n";
				}
				else if(segment == "static"){
					ofs << "@"+filename+"."+std::to_string(index)+"\nD=M\n";	
				}
				else if(segment == "pointer"){
					ofs << "@"+std::to_string(3+index)+"\nD=M\n";
				}
				else if(segment == "temp"){
					ofs << "@"+std::to_string(5+index)+"\nD=M\n";
				}
				else{
					const static std::unordered_map<std::string,std::string> segTable = {
						{"local","LCL"},
						{"argument","ARG"},
						{"this","THIS"},
						{"that","THAT"}
					};
					ofs << "@"+segTable.at(segment)+"\nD=M\n@"+std::to_string(index)+"\nA=D+A\nD=M\n";
				}
				// assume value to push is in D register
				ofs << "@SP\nAM=M+1\nA=A-1\nM=D\n"; 
				break;		
			}
			case Parser::C_POP:{
				if(segment == "constant"){
					break; // no-op
				}
				else if(segment == "static"){
					ofs << "@"+filename+"."+std::to_string(index)+"\n";	
				}
				else if(segment == "pointer"){
					ofs << "@"+std::to_string(3+index)+"\n";
				}
				else if(segment == "temp"){
					ofs << "@"+std::to_string(5+index)+"\n";
				}
				else{
					const static std::unordered_map<std::string,std::string> segTable = {
						{"local","LCL"},
						{"argument","ARG"},
						{"this","THIS"},
						{"that","THAT"}
					};
					ofs << "@"+segTable.at(segment)+"\nD=M\n@"+std::to_string(index)+"\nA=D+A\n";
				}
				// assume address for popped value is in A register
				ofs << "D=A\n@13\nM=D\n@SP\nAM=M-1\nD=M\n@13\nA=M\nM=D\n";
				break; 		
			}
			default:{
				throw std::runtime_error("Invalid command passed to writePushPop");
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
	std::string filename;
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
			return -1;
		}
	}
	else if(path_in.extension() != ".vm"){
		std::cout << "Bad extension : file extension must be \".vm\"\n";
		return -1;
	}
	else{
		files.push_back(path_in);
	}
	const std::string filename_out = path_in.stem().string() + ".asm";

	CodeWriter cw(filename_out);
	std::cout << "Start parsing..." << std::endl;
	for(auto&& v : files){
		Parser p(v.string());
		std::cout << "Parsing" << v.string() << std::endl;
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
/*
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
*/
			}
		}
	}
	std::cout << "Successfully Translated" << std::endl;
}
