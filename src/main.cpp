#include <iostream>
#include <fstream>

#include <string>
#include <vector>

#include "token.h"
#include "node.h"
#include "tac.h"

#define LEXER_DEBUG TOKEN_DEBUG
#define PARSER_DEBUG NODE_DEBUG & TOKEN_DEBUG
#define ASM 1

std::vector<Token>     lex(std::ifstream& file);
Node*                  parse();
void                   analyze(Node& node);
std::vector<TAC_Stmnt> TAC_Gen(Node& node);
void                   optimize(std::vector<TAC_Stmnt>& stmnts);
std::string            x86_Gen(std::vector<TAC_Stmnt>& stmnts);

bool error_reported = false;

void report_Error(const std::string& err, unsigned int line_num)
{
	std::cout << "\033[1;31mERROR: " << err << '\n';
	std::cout << "\033[1;38;5;124mON LINE " << line_num << '\n';
	std::cout << "\033[0m";

	error_reported = true;
}
void report_Warning(const std::string& warn, unsigned int line_num)
{
	std::cout << "\033[0;35mWARNING: " << warn << '\n';
	std::cout << "\033[0;38;5;129mon line " << line_num << '\n';
	std::cout << "\033[0m";
}

std::vector<Token> tokens; 

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cout << "File not specified, aborting\n";
		return 1;
	}
	std::ifstream file(argv[1]);
	std::ofstream write;

	tokens = lex(file);
#if LEXER_DEBUG
	write.open("lexer_output.txt");
	for(auto& token : tokens)
	{
		write << '<' << token.tc << ':' << token.line_num << ':' << token.str << '>' << '\n';
	}
	write.close();
#endif
	if(error_reported)
	{
		std::cout << "\033[1;38;5;202mlexing failed, aborting\033[0m\n";
		return 2;
	}
	Node* root = parse();
	if(error_reported)
	{
		std::cout << "\033[1;38;5;202mparsing failed, aborting\033[0m\n";
		delete root;
		return 3;
	}
#if PARSER_DEBUG
	write.open("parser_output.txt");
	write << *root;
	write.close();
#endif

	analyze(*root);
	if(error_reported)
	{
		std::cout << "\033[1;38;5;202msemantic analyzer failed, aborting\033[0m\n";
		delete root;
		return 4;
	}

	std::vector<TAC_Stmnt> tac = TAC_Gen(*root);
	delete root;

#if TAC_DEBUG
	write.open("TACGen_output.txt");
	for(auto& stmnt : tac)
	{
		write << stmnt << '\n';
	}
	write.close();
#endif
	
	//optimization
	optimize(tac);

#if TAC_DEBUG
	for(auto& stmnt : tac)
	{
		//std::cout << stmnt << '\n';
	}
#endif

	//std::cout << "\nx86 asm: \n\n";
	//std::cout << x86_Gen(tac) << '\n';
	std::string output = "section\t.data\n"
	                     "\nsection\t.text\n"
	                     "global \t_start\n"
	                     "_start:\n"
	                     "\tcall Fmain\n"
	                     "\tmov \trdi, rax\n"
	                     "\tmov \trax, 60\n" 	
	                     "\tsyscall\n\n";
	output += x86_Gen(tac);
	write.open("temp.asm");
	write << output << '\n';
	write.close();
	
	system("nasm -f elf64 -o temp.o temp.asm");
	system("ld -o program temp.o");
	
#if ! ASM
	system("rm -f temp.asm");
#endif
	system("rm -f temp.o");
}

