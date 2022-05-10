#include <vector>
#include <string>

#include "token.h"
#include "node.h"
#include "tac.h"

TAC_Stmnt::TAC_Stmnt()
	{}
//labels
TAC_Stmnt::TAC_Stmnt(const std::string& n_label)
	: str(n_label), tt(TAC_Type::label) {}
//funccall
TAC_Stmnt::TAC_Stmnt(unsigned int n_dest, const std::string& n_name, const std::vector<unsigned int>& n_params)
	: str(n_name), dest(n_dest), tt(TAC_Type::fcall), params(n_params)  {}
//return
TAC_Stmnt::TAC_Stmnt(int src, bool is_reg)
	: tt(TAC_Type::retur)
{
	if(is_reg) src1 = src;
	else { src1 = -1; val1 = src; }
}
//if
TAC_Stmnt::TAC_Stmnt(const std::string& n_label, int src, bool is_reg)
	: str(n_label), tt(TAC_Type::ifblk)
{
	if(is_reg) src1 = src;
	else { src1 = -1; val1 = src; }
}
//assignment
TAC_Stmnt::TAC_Stmnt(unsigned int n_dest, int src, bool is_reg)
	: dest(n_dest), tt(TAC_Type::assig)
{
	if(is_reg) src1 = src;
	else { src1 = -1; val1 = src; }
}
//string assignment
TAC_Stmnt::TAC_Stmnt(unsigned int n_dest, const std::string& n_str)
	: str(n_str), dest(n_dest), tt(TAC_Type::asstr) {}
//arithmetic
TAC_Stmnt::TAC_Stmnt(unsigned int n_dest, int n_src1, bool is_reg1, int n_src2, bool is_reg2, TAC_Type ntt)
	: dest(n_dest), tt(ntt)
{
	if(is_reg1) src1 = n_src1;
	else { src1 = -1; val1 = n_src1; }
	if(is_reg2) src2 = n_src2;
	else { src2 = -1; val2 = n_src2; }
}

#if TAC_DEBUG
std::ostream& operator<<(std::ostream& os, TAC_Stmnt& ts)
{
	std::string oper;
	switch(ts.tt)
	{
	case TAC_Type::label:
		os << ts.str << ':';
		break;
	case TAC_Type::fcall:
		os << "\tT" << ts.dest << " = " << ts.str << '(';
		{ bool more_than_one = false;
		for(auto& param : ts.params)
		{
			if(more_than_one) os << ", ";
			os << 'T' << param;
			more_than_one = true;
		}}
		os << ')';
		break;
	case TAC_Type::assig:
		os << "\tT" << ts.dest << " = ";
		if(ts.src1 == -1)
		{
			os << ts.val1; 
		}
		else
		{
			os << 'T' << ts.src1;
		}
		
		break;
	case TAC_Type::asstr:
		os << "\tT" << ts.dest << " = " << ts.str;
		break;
	case TAC_Type::retur:
		os << "\tret ";
		if(ts.src1 == -1)
		{
			os << ts.val1; 
		}
		else
		{
			os << 'T' << ts.src1;
		}
		break;
	case TAC_Type::ifblk:
		os << "\tjze ";
		if(ts.src1 == -1)
		{
			os << ts.val1; 
		}
		else
		{
			os << 'T' << ts.src1;
		}
		os << ' ' << ts.str;
		break;
	case TAC_Type::armul:
		oper = " * ";
		goto add_oper;
	case TAC_Type::ardiv:
		oper = " / ";
		goto add_oper;
	case TAC_Type::aradd:
		oper = " + ";
		goto add_oper;
	case TAC_Type::arsub:
		oper = " - ";
		goto add_oper;
	case TAC_Type::arequ:
		oper = " == ";
		goto add_oper;
	case TAC_Type::arneq:
		oper = " != ";
		goto add_oper;
	case TAC_Type::arles:
		oper = " < ";
		goto add_oper;
	case TAC_Type::argrt:
		oper = " > ";
		goto add_oper;
	add_oper:
		os << "\tT" << ts.dest << " = ";
		if(ts.src1 == -1)
		{
			os << ts.val1; 
		}
		else
		{
			os << 'T' << ts.src1;
		}
		os << oper;
		if(ts.src2 == -1)
		{
			os << ts.val2; 
		}
		else
		{
			os << 'T' << ts.src2;
		}
		break;
	}

	return os;
}
#endif

//index of str determines what reg name it will use
std::vector<std::string> env;
std::vector<TAC_Stmnt>   stmnts;


void func_Gen(Node& node);
void stmt_Gen(Node& node);
void expr_Gen(Node& node);
unsigned int var_Loc(const std::string& name);
unsigned int last_label = 0;


std::vector<TAC_Stmnt> TAC_Gen(Node& node)
{
	for(auto& child : node.children)
	{
		func_Gen(*child);
	}

	std::vector<TAC_Stmnt> stmnts_cpy = stmnts;	
	stmnts.clear();
	return stmnts_cpy;
}

void func_Gen(Node& node)
{
	//each time we have new env so that registers names wont interfere with each other
	env.clear();
	//adds func name as label
	stmnts.emplace_back(std::string("F") + node.children[1]->tkn.str);
	stmnts.back().param_amount = node.children[2]->children.size();
	//adds func parameters to env as names
	for(auto& param : node.children[2]->children)
	{
		env.emplace_back(param->tkn.str);
	}

	stmt_Gen(*node.children[3]);
}

void stmt_Gen(Node& node)
{
	switch(node.children[0]->nt)
	{
	case Node_Type::stms:
	{
		//to reduce reg nums after scope 
		unsigned int last_num_saved = env.size();
		
		for(auto& child : node.children[0]->children)
		{
			 stmt_Gen(*child);
		}
	
		while(env.size() > last_num_saved)
		{
			env.pop_back();
		}
		break;
	}
	case Node_Type::tokn:
	{
		switch(node.children[0]->tkn.tc)
		{
		case Token_Class::kret:
			//generates result of expr and puts it in reg that will be used by return
			//to ensure that it has only one arg
			//similar things are done in each TOKEN_CLASS
			//and will be generally optimized
			expr_Gen(*node.children[1]);
			stmnts.emplace_back(env.size() - 1, TAC_REG);
			break;

		case Token_Class::kvar:
		{
			expr_Gen(*node.children[2]);
			unsigned int loc1 = env.size() - 1;
			env.emplace_back(node.children[1]->tkn.str);
			stmnts.emplace_back(env.size() - 1, loc1, TAC_REG);
			break;
		}
		case Token_Class::kif:
		{
			std::string label = "L";
			label += std::to_string(last_label++);

			expr_Gen(*node.children[1]);
			unsigned int loc1 = env.size() - 1;
			stmnts.emplace_back(label, loc1, TAC_REG);
			stmt_Gen(*node.children[2]);
			stmnts.emplace_back(label);
			break;
		}
		default:
			break;
		}
		break;

	}
	case Node_Type::expr:
		expr_Gen(*node.children[0]);
		break;
	default:
		break;
	}
}

void expr_Gen(Node& node)
{
	switch(node.children.size())
	{
	case 1:
		if(Node_Type::tokn == node.children[0]->nt)
		{
			Token& token = node.children[0]->tkn;
			env.emplace_back("");
			//creates reg and puts literal / immediate there for furthes use
			//due to how other TAC gen is structured it adds one reg
			//will be generally optimized 
			if(Token_Class::numb == token.tc) stmnts.emplace_back(env.size() - 1, std::stoi(token.str), TAC_CNS);
			else if(Token_Class::strn == token.tc) stmnts.emplace_back(env.size() - 1, token.str);
			else if(Token_Class::iden == token.tc)
			{
				stmnts.emplace_back(env.size() - 1, var_Loc(token.str), TAC_REG);
			}
		}
		//in paretheses
		else
		{
			expr_Gen(*node.children[0]);
		}
		break;
	case 2:
	{
		//function calls
		//emplaces each arg in register that will be later used by funccall
		std::vector<unsigned int> locs;
		for(auto& arg : node.children[1]->children)
		{
			expr_Gen(*arg);
			locs.emplace_back(env.size() - 1);
		}
		env.emplace_back("");
		unsigned int loc1 = env.size() - 1;
		stmnts.emplace_back(loc1, std::string("F") + node.children[0]->tkn.str, locs);
	}
		break;
	case 3:
	{
		//set expr
		if(Node_Type::tokn == node.children[0]->nt)
		{
			expr_Gen(*node.children[2]);	
			unsigned int loc1 = env.size() - 1;
			stmnts.emplace_back(var_Loc(node.children[0]->tkn.str), loc1, TAC_REG);
		}
		//aritmetic expr
		else
		{
			//generates each expr and puts it into regs before generating this expr
			//to ensure that only two operands are here
			//again, will be generally optimized
			expr_Gen(*node.children[0]);
			unsigned int loc1 = env.size() - 1;
			expr_Gen(*node.children[2]);
			unsigned int loc2 = env.size() - 1;
			env.emplace_back("");
			TAC_Type tt;
			switch(node.children[1]->tkn.tc)
			{
			case Token_Class::omul:
				tt = TAC_Type::armul;
				break;
			case Token_Class::odiv:
				tt = TAC_Type::ardiv;
				break;
			case Token_Class::oadd:
				tt = TAC_Type::aradd;
				break;
			case Token_Class::osub:
				tt = TAC_Type::arsub;
				break;
			case Token_Class::oequ:
				tt = TAC_Type::arequ;
				break;
			case Token_Class::oneq:
				tt = TAC_Type::arneq;
				break;
			case Token_Class::ogrt:
				tt = TAC_Type::argrt;
				break;
			case Token_Class::oles:
				tt = TAC_Type::arles;
				break;
			default:
				break;
			}
			stmnts.emplace_back(env.size() - 1, loc1, TAC_REG, loc2, TAC_REG, tt); 
		}
	}
		break;
	}

}

unsigned int var_Loc(const std::string& name)
{
	for(int i = env.size() - 1; i >= 0; i--)
	{
		if(env[i] == name)
			return i;
	}
	//just so compiler doesnt complain
	//in practice impossible to get here because semantic analysis
	return 0;
}
