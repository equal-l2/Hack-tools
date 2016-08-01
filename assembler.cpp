#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <unordered_map>
#include <algorithm>
#include <regex>

using add_t = unsigned;

bool is_number(const std::string& s){
	return !s.empty() && (std::find_if(s.begin(),s.end(),[](char c){ return !isdigit(c);}) == s.end());
}

class Parser{
public:
	enum enumType {A_COMMAND,C_COMMAND,L_COMMAND};

	Parser(std::string filename):ifs(filename){
		if(!ifs.is_open()) throw std::runtime_error("File is not found\n");
	}
	bool hasMoreCommands(){
		return parse_core();
	}
	void advance(){
		; 
		// intentionally no-op
		// since everytime this function is called, hasMoreCommands is called beforehand
		// and hasMoreCommands parses source actually in my implementation
	}
	enumType commandType(){
		return type;
	}
	std::string symbol(){
		return symbol_str;
	}
	std::string dest(){
		return dest_str;
	}
	std::string comp(){
		return comp_str;
	}
	std::string jump(){
		return jump_str;
	}
private:
	std::ifstream ifs;
	std::string symbol_str;
	std::string dest_str;
	std::string comp_str;
	std::string jump_str;
	enumType type;

	bool parse_core(){
		symbol_str = dest_str = comp_str = jump_str = "null";
		std::string buf;

		READ_VAL:
		if(!std::getline(ifs,buf)) return false;

		buf.erase(remove_if(buf.begin(), buf.end(), isspace), buf.end()); // remove spaces

		auto n = buf.find("//");
		if(n != buf.npos){ // if comment exists
			buf.erase(n);
		}

		if(buf == "\n" || buf == "") goto READ_VAL;

		try{
			regex_core(buf);
		}
		catch(std::runtime_error& e){
			std::cout << e.what() << std::endl;
			throw e;
		}
		return true;
	}

	void regex_core(std::string str){
		const std::string symbol_match = "([a-zA-Z_.$:][a-zA-Z\\d_.$:]*)";
		const std::string dest_match = "([AMD]|A[MD]|[A]?MD)";
		const std::string comp_match = "(0|[-]?1|[-!]?[DAM]|D[-+&|][AM]|[AMD][-+]1|[AM]-D)";
		const std::string jump_match = "(JGT|JEQ|JGE|JLT|JNE|JLE|JMP)";

		const std::regex a_match1("@" + symbol_match);
		const std::regex a_match2("@([\\d]*)");
		const std::regex c_match1(dest_match + "=" + comp_match + ";" + jump_match + "?");
		const std::regex c_match2(dest_match + "=" + comp_match + ";?");
		const std::regex c_match3(comp_match + ";" + jump_match + "?");
		const std::regex c_match4(jump_match);
		const std::regex l_match("\\(" + symbol_match + "\\)");

		std::smatch match;
		if(std::regex_match(str,match,a_match1)){
			if (match.size() == 2) {
				symbol_str = match[1].str();
				type = A_COMMAND;
				return;
			}
			else{
				throw std::runtime_error("A_COMMAND regex error 1");
			}
		}
		if(std::regex_match(str,match,a_match2)){
			if (match.size() == 2) {
				symbol_str = match[1].str();
				type = A_COMMAND;
				return;
			}
			else{
				throw std::runtime_error("A_COMMAND regex error 2");
			}
		}
		else if(std::regex_match(str,match,l_match)){
			if (match.size() == 2) {
				symbol_str = match[1].str();
				type = L_COMMAND;
				return;
			}
			else{
				throw std::runtime_error("L_COMMAND regex error");
			}
		}
		else if(std::regex_match(str,match,c_match1)){
			if (match.size() == 4) {
				dest_str = match[1].str();
				comp_str = match[2].str();
				jump_str = match[3].str();
				type = C_COMMAND;
				return;
			}
			else{
				throw std::runtime_error("C_COMMAND regex error 1");
			}
		}
		else if(std::regex_match(str,match,c_match2)){
			if (match.size() == 3) {
				dest_str = match[1].str();
				comp_str = match[2].str();
				type = C_COMMAND;
				return;
			}
			else{
				throw std::runtime_error("C_COMMAND regex error 2");
			}
		}
		else if(std::regex_match(str,match,c_match3)){
			if (match.size() == 3) {
				comp_str = match[1].str();
				jump_str = match[2].str();
				type = C_COMMAND;
				return;
			}
			else{
				throw std::runtime_error("C_COMMAND regex error 3");
			}
		}
		else if(std::regex_match(str,match,c_match4)){
			if (match.size() == 2) {
				jump_str = match[1].str();
				type = C_COMMAND;
				return;
			}
			else{
				throw std::runtime_error("C_COMMAND regex error 4");
			}
		}
		else{
			throw std::runtime_error("Unknown instruction\n\""+str+"\"\n");
		}
	}
};

class Code{
public:
	static std::bitset<3> dest(std::string mnemo){
		const std::unordered_map<std::string,unsigned> binTable = {
			{"null"	,0},
			{"M"	,1},
			{"D"	,2},
			{"MD"	,3},
			{"A"	,4},
			{"AM"	,5},
			{"AD"	,6},
			{"AMD"	,7}
		};
		return binTable.at(mnemo);
	}
	static std::bitset<7> comp(std::string mnemo){
		const std::unordered_map<std::string,std::string> binTable = {
			{"0"	,"0101010"},
			{"1"	,"0111111"},
			{"-1"	,"0111010"},
			{"D"	,"0001100"},
			{"A"	,"0110000"},
			{"!D"	,"0001101"},
			{"!A"	,"0110001"},
			{"-D"	,"0001111"},
			{"-A"	,"0110011"},
			{"D+1"	,"0011111"},
			{"A+1"	,"0110111"},
			{"D-1"	,"0001110"},
			{"A-1"	,"0110010"},
			{"D+A"	,"0000010"},
			{"D-A"	,"0010011"},
			{"A-D"	,"0000111"},
			{"D&A"	,"0000000"},
			{"D|A"	,"0010101"},
			{"M"	,"1110000"},
			{"!M"	,"1110001"},
			{"-M"	,"1110011"},
			{"M+1"	,"1110111"},
			{"M-1"	,"1110010"},
			{"D+M"	,"1000010"},
			{"D-M"	,"1010011"},
			{"M-D"	,"1000111"},
			{"D&M"	,"1000000"},
			{"D|M"	,"1010101"},
		};
		return std::bitset<7>(binTable.at(mnemo));
	}
	static std::bitset<3> jump(std::string mnemo){
		const std::unordered_map<std::string,unsigned> binTable = {
			{"null"	,0},
			{"JGT"	,1},
			{"JEQ"	,2},
			{"JGE"	,3},
			{"JLT"	,4},
			{"JNE"	,5},
			{"JLE"	,6},
			{"JMP"	,7}
		};
		return binTable.at(mnemo);
	}
};

class SymbolTable{
public:
	SymbolTable():table({
		{"SP",0},
		{"LCL",1},
		{"ARG",2},
		{"THIS",3},
		{"THAT",4},
		{"R0",0},
		{"R1",1},
		{"R2",2},
		{"R3",3},
		{"R4",4},
		{"R5",5},
		{"R6",6},
		{"R7",7},
		{"R8",8},
		{"R9",9},
		{"R10",10},
		{"R11",11},
		{"R12",12},
		{"R13",13},
		{"R14",14},
		{"R15",15},
		{"SCREEN",16384},
		{"KBD",24576}
	}){}

	void addEntry(std::string symbol, add_t address){
		table.insert({symbol,address});
	}

	bool contains(std::string symbol){
		return (table.find(symbol) != table.end());
	}

	add_t getAddress(std::string symbol){
		return table.at(symbol);
	}

	void print(){
		for(auto elem : table){
			std::cout << elem.first << " " << elem.second << "\n";
		}
	}
private:
	std::unordered_map<std::string,add_t> table;
};

int main(int argc, char** argv){

	if(argc < 2){
		std::cout << "Usage : Assembler <filename>\n";
		return 0;
	}

	const std::string filename_in = argv[1];
	const std::regex filename_check(".*\\.asm");
	if(!std::regex_match(filename_in,filename_check)){
		std::cout << "Bad file extension : file name must end with \".asm\"\n";
	}
	const std::string filename_out = filename_in.substr(0,filename_in.size()-4) + ".hack";

	try{
		SymbolTable s;

		/* First Path */
		std::cout << "First Path..." << std::endl;
		{
			Parser p(filename_in);
			//add_t cnt=0;
			add_t rom_add = 0;
			while(p.hasMoreCommands()){
				//if(cnt%100 == 0) std::cout << cnt;
				p.advance();
				switch(p.commandType()){
					case Parser::A_COMMAND:{
						++rom_add;
						break;
					}
					case Parser::C_COMMAND:{
						++rom_add;
						break;
					}
					case Parser::L_COMMAND:{
						auto buf = p.symbol();
						s.addEntry(buf,rom_add);
						break;
					}
				}
				//++cnt;
				//std::cout << '\r' << std::flush;
			}
		}
		std::cout << "First Pass Ended..." << std::endl;

		/* Second Path */
		std::ofstream ofs(filename_out);
		{
			Parser p(filename_in);
			//add_t cnt = 0;
			add_t var_add = 16;
			while(p.hasMoreCommands()){
				//if(cnt%100 == 0) std::cout << cnt;
				p.advance();
				std::string output;
				switch(p.commandType()){
					case Parser::A_COMMAND:{
						auto buf = p.symbol();
						add_t add;
						if(is_number(buf)){
							add = std::stoul(buf);
						}
						else{
							if(s.contains(buf)){
								add = s.getAddress(buf);
							}
							else{
								s.addEntry(buf,var_add);
								add = var_add++;
							}
						}
						output = std::bitset<16>(add).to_string();
						break;
					}
					case Parser::C_COMMAND:{
						output = "111";
						output += Code::comp(p.comp()).to_string() + Code::dest(p.dest()).to_string() + Code::jump(p.jump()).to_string();
						break;
					}
					case Parser::L_COMMAND:{
						goto END;
					}
				}
				ofs << output << '\r';
				END: ;
				//++cnt;
				//std::cout << '\r' << std::flush;
			}
		}
		std::cout << "Successfully Assembled" << std::endl;
		ofs.close();
	}
	catch(const std::runtime_error& e){
		std::cout << e.what();
		return 1;
	}
}
