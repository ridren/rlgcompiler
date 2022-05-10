#include <fstream>

#include <string>
#include <vector>

#include "token.h"

void report_Error(const std::string& err, unsigned int line_num);
void report_Warning(const std::string& warn, unsigned int line_num);

#if TOKEN_DEBUG
std::ostream& operator<<(std::ostream& os, Token_Class tc)
{
	using enum Token_Class;

	switch(tc)
	{
	case kfnc:
		os << "KFNC";
		break;
	case kret:
		os << "KRET";
		break;
	case kvar: 
		os << "KVAR"; 
		break;
	case kint: 
		os << "KINT"; 
		break;
	case kvod: 
		os << "KVOD"; 
		break;
	case kif: 
		os << "KIF "; 
		break;
	case kop: 
		os << "KOP "; 
		break;
	case coma: 
		os << "COMA"; 
		break;
	case coln: 
		os << "COLN"; 
		break;
	case scln: 
		os << "SCLN"; 
		break;
	case prnl: 
		os << "PRNL"; 
		break;
	case prnr: 
		os << "PRNR"; 
		break;
	case brkl: 
		os << "BRKL"; 
		break;
	case brkr: 
		os << "BRKR"; 
		break;
	case numb: 
		os << "NUMB"; 
		break;
	case strn: 
		os << "STRN"; 
		break;
	case iden: 
		os << "IDEN"; 
		break;
	case omul: 
		os << "OMUL"; 
		break;
	case odiv: 
		os << "ODIV"; 
		break;
	case oadd: 
		os << "OADD"; 
		break;
	case osub: 
		os << "OSUB"; 
		break;
	case oset: 
		os << "OSET"; 
		break;
	case oequ: 
		os << "OEQU"; 
		break;
	case oneq: 
		os << "ONEQ"; 
		break;
	case ogrt: 
		os << "OGRT"; 
		break;
	case oles: 
		os << "OLES"; 
		break;
	case enof: 
		os << "ENOF"; 
		break;
	}

	return os;
}
#endif

Token::Token() {}
Token::Token(Token_Class n_tc, const std::string& n_str, unsigned int n_line)
	:str(n_str), line_num(n_line), tc(n_tc) {}

Token::Token(Token_Class n_tc, char n_chr, unsigned int n_line)
	: str(1, n_chr), line_num(n_line), tc(n_tc) {}


std::vector<Token> lex(std::ifstream& file)
{
	std::vector<Token> tokens;

	std::string line;
	unsigned int line_num = 0;

	std::string to_add;
	Token_Class tc;
	int paren_count = 0; //at end of each stmnt should be 0
	//tokenize each line
	while(std::getline(file, line))
	{
		line_num++;
		
		for(unsigned int i = 0; i < line.length(); i++)
		{
			switch(line[i])
			{
			//comment
			case '#':
				i = line.length();
				continue;
			//whitespace
			case ' ':
			case '\t':
				if(to_add.length() > 0)
				{
					tokens.emplace_back(Token_Class::iden, to_add, line_num);
					to_add.clear();
				}
				break;	
			//expr
			case '=':
				//check if "=" xor "=="
				if(line[i + 1] == '=')
				{
					i++;
					tc = Token_Class::oequ;
				}
				else
				{
					tc = Token_Class::oset;
				}
				goto add_one_char;
			case '!':
				if(line[i + 1] == '=')
				{
					i++;
					tc = Token_Class::oneq;
					goto add_one_char;
				}
				else goto def;
			case '*':
				tc = Token_Class::omul;
				goto add_one_char;
			case '/':
				tc = Token_Class::odiv;
				goto add_one_char;
			case '+':
				tc = Token_Class::oadd;
				goto add_one_char;
			case '-':
				tc = Token_Class::osub;
				goto add_one_char;
			case '<':
				tc = Token_Class::ogrt;
				goto add_one_char;
			case '>':
				tc = Token_Class::oles;
				goto add_one_char;

			//one char literals
			case ',':
				tc = Token_Class::coma;
				goto add_one_char;
			case ':':
				tc = Token_Class::coln;
				goto add_one_char;
			case ';':
				if(0 != paren_count)
				{
					report_Error("UNMATCHED PARENTHESIS IN STMNT ENDING", line_num);
					paren_count = 0;
				}
				tc = Token_Class::scln;
				goto add_one_char;
			case '(':
				paren_count++;
				tc = Token_Class::prnl;
				goto add_one_char;
			case ')':
				paren_count--;
				tc = Token_Class::prnr;
				goto add_one_char;
			case '{':
				tc = Token_Class::brkl;
				goto add_one_char;
			case '}':
				if(0 != paren_count)
				{
					report_Error("UNMATCHED PARENTHESIS IN STMNT ENDING", line_num);
					paren_count = 0;
				}
				tc = Token_Class::brkr;
			add_one_char:
				if(to_add.length() > 0)
				{
					tokens.emplace_back(Token_Class::iden, to_add, line_num);
					to_add.clear();
				}
				tokens.emplace_back(tc, "", line_num);
				break;
			//numbers
			case '0'...'9':
				if(to_add.length() > 0)
				{
					to_add += line[i];
				}
				else
				{
					while(line[i] >= '0' && line[i] <= '9') 
					{
						to_add += line[i];
						i++;
					}
					tokens.emplace_back(Token_Class::numb, to_add, line_num);
					to_add.clear();
				}
				i--;
				break;
			//strings
			case '"':
				if(to_add.length() > 0)
				{
					tokens.emplace_back(Token_Class::iden, to_add, line_num);
					to_add.clear();
				}
				i++;
				while(i < line.size())
				{
					//if escape char
					if(line[i] == '\\')
					{	
						switch(line[i + 1])
						{
							case '"':
								i++;
								to_add += line[i];
								break;
						}
					}
					else
					{
						//if str finished
						if(line[i] == '"')
						{
							tokens.emplace_back(Token_Class::strn, to_add, line_num);
							to_add.clear();
							goto string_add_good;
						}
						
						to_add += line[i];
					}
					i++;
				}
				report_Error("UNTERMINATED STRING", line_num);

			string_add_good:
				break;

			//kwrds & identifiers
def:
			default:
				to_add += line[i];
			}
		}
	}
	if(to_add.length() > 0)
	{
		tokens.emplace_back(Token_Class::iden, to_add, line_num);
		to_add.clear();
	}
	tokens.emplace_back(Token_Class::enof, "", line_num + 1);
	
	//changes identifiers to keywords 
	for(auto& token : tokens)
	{
		if(Token_Class::iden == token.tc)
		{
			if("func" == token.str)
			{
				token.tc = Token_Class::kfnc;
				token.str.clear();
			}
			else if("return" == token.str)
			{
				token.tc = Token_Class::kret;
				token.str.clear();
			}
			else if("var" == token.str)
			{
				token.tc = Token_Class::kvar;
				token.str.clear();
			}
			else if("int" == token.str)
			{
				token.tc = Token_Class::kint;
				token.str.clear();
			}
			else if("void" == token.str)
			{
				token.tc = Token_Class::kvod;
				token.str.clear();
			}
			else if("if" == token.str)
			{
				token.tc = Token_Class::kif;
				token.str.clear();
			}
			else if("op" == token.str)
			{
				token.tc = Token_Class::kop;
				token.str.clear();
			}
		}
	}

	return tokens;
}
