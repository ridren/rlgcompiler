#include <vector>

#include "token.h"
#include "node.h"

void report_Error(const std::string& err, unsigned int line_num);
void report_Warning(const std::string& warn, unsigned int line_num);

struct Symbol{
	std::string name;
	unsigned int arg_count;
	bool scope_sign = false; //used to determine where does scope end
	bool is_var;
	bool does_return; //to check whether func return value or not

	Symbol(bool is_scope_sign)
		: scope_sign(is_scope_sign), is_var(false) {}
	Symbol(const std::string& n_name, bool n_is_var, unsigned int n_arg_count = 0, bool n_does_return = false)
		: name(n_name), arg_count(n_arg_count), is_var(n_is_var), does_return(n_does_return) {}
};

std::vector<Symbol> symbols;
void enter_Scope();
void push_Symbol(const std::string& name, bool is_var, unsigned int arg_count = 0, bool does_return = false);
bool find_Symbol(const std::string& name, bool is_var);
bool check_Scope(const std::string& name, bool is_var);
void exit_Scope();

int found_symbol_index = -1;
bool expects_return = true;
bool func_returns = false;

void analyze(Node& node)
{
	switch(node.nt)
	{
	case Node_Type::tokn:
	{
		//check if variable exists in scope
		//check for functions is done later
		if(Token_Class::iden == node.tkn.tc)
		{
			if(!find_Symbol(node.tkn.str, true))
				report_Error(node.tkn.str + " NOT DECLARED IN THIS SCOPE", node.tkn.line_num); 
		}
		return;
	}
	case Node_Type::prog:
	{
		//analyzes each function
		for(auto& child : node.children)
			analyze(*child);
		return;
	}
	case Node_Type::func:
	{
		Token& token = node.children[1]->tkn;
		//check for redefinition of func
		if(find_Symbol(token.str, false))
		{
			report_Error(token.str + " REDEFINITION", token.line_num);
			return;
		}
		//if no definition, add func
		bool returns = Token_Class::kint == node.children[0]->tkn.tc;
		push_Symbol(node.children[1]->tkn.str, false, node.children[2]->children.size(), returns);

		enter_Scope();
		//checking and adding params as variables
		for(auto& param : node.children[2]->children)
		{
			if(check_Scope(param->tkn.str, true))
			{
				report_Error(param->tkn.str + " REDECLARED", param->tkn.line_num);
				continue;
			}
			push_Symbol(param->tkn.str, true);
		}
		func_returns = false;

		//analyze stmnt of func
		analyze(*node.children[3]);
	
		if(returns && !func_returns)
		{
			report_Warning(node.children[1]->tkn.str + " should return value", node.children[1]->tkn.line_num);
		}
		if(!returns && func_returns)
		{
			report_Warning(node.children[1]->tkn.str + " should not return value", node.children[1]->tkn.line_num);
		}

		exit_Scope();
		break;
	}
	case Node_Type::stmn:
	{
		//first child of stmnt can show stmnt type
		switch(node.children[0]->nt)
		{
		//each compound stmnt creates another scope inside itself
		case Node_Type::stms:
			enter_Scope();
			for(auto& child : node.children[0]->children)
				analyze(*child);
			exit_Scope();
			break;
		case Node_Type::tokn:
		{
			switch(node.children[0]->tkn.tc)
			{
			case Token_Class::kret:
				analyze(*node.children[1]);
				func_returns = true;
				break;
			case Token_Class::kif:
				analyze(*node.children[1]);
				enter_Scope();
				analyze(*node.children[2]);
				exit_Scope();
				break;
			case Token_Class::kvar:
			{
				Token& token = node.children[1]->tkn;
				if(check_Scope(token.str, true))
				{
					report_Error(token.str + " REDEFINED", token.line_num);
					break;
				}
				push_Symbol(token.str, true);
			}
			//no op stmnt ommited since it is always correct
			default:
				break;
			}
			break;;
		}
		case Node_Type::expr:
			expects_return = false;
			analyze(*node.children[0]);
			expects_return = true;
			break;;
		default:
			break;
		}
		return;
	}
	case Node_Type::expr:
		//literal, identifier or ( EXPR ) 
		if(1 == node.children.size())
		{
			expects_return = true;
			analyze(*node.children[0]);
			break;
		}
		//func call
		if(2 == node.children.size())
		{
			Token& token = node.children[0]->tkn;
			if(!find_Symbol(token.str, false))
			{
				report_Error(token.str + " FUNCTION UNDEFINED", token.line_num);
				return;
			}
			if(symbols[found_symbol_index].arg_count != node.children[1]->children.size())
			{
				std::string err = token.str + " TAKES ";
				err += std::to_string(symbols[found_symbol_index].arg_count) + " ARGS, HOWEVER ";
				err += std::to_string(node.children[1]->children.size()) + " PROVIDED";
				report_Error(err, token.line_num);
				return;
			}
			if(expects_return && !symbols[found_symbol_index].does_return)
			{
				report_Error(token.str + " FUNCTION DOES NOT RETURN ANY VALUE, YET IT IS REQUIRED IN CONTEXT", token.line_num);
				return;
			}
			for(auto& child : node.children[1]->children)
			{
				expects_return = true;
				analyze(*child);
			}
			break;
		}	
		//assignment or arithmetic
		if(3 == node.children.size())
		{
			expects_return = true;
			analyze(*node.children[0]);
			analyze(*node.children[2]);
		}
		break;
	default:
		break;
	}
}


void enter_Scope()
{
	symbols.emplace_back(true);
}

void push_Symbol(const std::string& name, bool is_var, unsigned int arg_count, bool does_return)
{
	symbols.emplace_back(name, is_var, arg_count, does_return);
}
bool find_Symbol(const std::string& name, bool is_var)
{
	for(int i = symbols.size() - 1; i >= 0; i--)
	{
		if(symbols[i].name   == name
		&& symbols[i].is_var == is_var)
		{
			found_symbol_index = i;
			return true;
		}
	}
	return false;
}
bool check_Scope(const std::string& name, bool is_var)
{
	for(int i = symbols.size() - 1; i >= 0; i--)
	{
		if(symbols[i].scope_sign)
			return false;

		if(symbols[i].name   == name
		&& symbols[i].is_var == is_var)
			return true;
	}
	return false;
}

void exit_Scope()
{	
	for(int i = symbols.size() - 1; i >= 0; i--)
	{
		if(symbols[i].scope_sign)
		{
			symbols.pop_back();
			break;
		}
		symbols.pop_back();
		
	}
}
