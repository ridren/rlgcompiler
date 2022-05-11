#include <string>
#include <vector>

#include "token.h"
#include "node.h"

void report_Error(const std::string& err, unsigned int line_num);
void report_Warning(const std::string& warn, unsigned int line_num);

Node::Node() {}
Node::Node(Node_Type n_nt)
	: nt(n_nt) {}
Node::Node(Token& n_tkn)
	: tkn(n_tkn), nt(Node_Type::tokn) {}
Node::~Node()
{
	for(auto& child : children)
	{
		delete child;
	}
}

#if NODE_DEBUG
std::ostream& operator<<(std::ostream& os, Node_Type nt)
{
	using enum Node_Type;

	switch(nt)
	{
	case prog:
		os << "PROG";
		break;
	case func:
		os << "FUNC";
		break;
	case rtyp:
		os << "RTYP";
		break;
	case prms:
		os << "PRMS";
		break;
	case stmn:
		os << "STMN";
		break;
	case stms:
		os << "STMS";
		break;
	case expr:
		os << "EXPR";
		break;
	case args:
		os << "ARGS";
		break;
	case tokn:
		os << "TOKN";
		break;
	}

	return os;
}

int intendation_level = 0;
bool last_child = false;
std::ostream& operator<<(std::ostream& os, Node& node)
{
	for(int i = 0; i < intendation_level - 1; i++)
	{
		os << ' ';
	}
	if(intendation_level != 0)
		os << (last_child ? "└" : "├");
	last_child = false;

	os << "\033[1;38;5;21m";
	os << node.nt << ' ';	
	if(Node_Type::tokn == node.nt)
	{
		os << "\033[0;38;5;93m" << node.tkn.tc;
		os << "\033[0;38;5;128m " << node.tkn.str;
	}
	os << "\n\033[0m";
	intendation_level++;
	for(auto& childptr : node.children)
	{
		if(&childptr == &node.children.back())
			last_child = true;

		os << *childptr;
	}
	intendation_level--;
	return os;
}
#endif

extern std::vector<Token> tokens;
unsigned int cur_token = -1;

Node* check_Func();
Node* check_RType();
Node* check_Params();
Node* check_Stmnt();
Node* check_Stmns();
Node* check_Expr();
Node* check_Args();
bool  check_Next_Token(Token_Class tc);

Node* parse()
{
	//this translation is not 1:1 with grammar 
	//it adds funcs as children to program
	//more direct translation would add subtress	
	//PROGRAM → FUNC PROGRAM
	//        | FUNC eof
	Node* program = new Node(Node_Type::prog);
	while(!check_Next_Token(Token_Class::enof))
	{
		Node* child = check_Func();
		if(nullptr == child)
		{
			delete program;
			return nullptr;
		}

		program->children.emplace_back(child);
	}

	return program;
}

Node* check_Func()
{
	Node* func   = new Node(Node_Type::func);

	int start_token = cur_token;
	if(-1 == start_token) start_token = 0;
	//FUNC    → "func" RTYPE ":" id "(" PARAMS ")" STMNT
	if(!check_Next_Token(Token_Class::kfnc))
	{
		report_Error("EXPECTED \"FUNC\"", tokens[cur_token].line_num);
		goto func_failed;
	}
	if(!func->children.emplace_back(check_RType()))
	{
		report_Error("EXPECTED RETURN TYPE AFTER \"FUNC\"", tokens[cur_token].line_num);
		goto func_failed;
	}
	if(!check_Next_Token(Token_Class::coln))
	{
		report_Error("EXPECTED ':' AFTER RETURN TYPE", tokens[cur_token].line_num);
		goto func_failed;
	}
	if(!check_Next_Token(Token_Class::iden))
	{
		report_Error("EXPECTED ID AFTER ':'", tokens[cur_token].line_num);
		goto func_failed;
	}
	
	func->children.emplace_back(new Node(tokens[cur_token]));
	
	if(!check_Next_Token(Token_Class::prnl))
	{
		report_Error("EXPECTED '(' AFTER ID", tokens[cur_token].line_num);
		goto func_failed;
	}
	if(!func->children.emplace_back(check_Params()))
	{
		report_Error("EXPECTED PARAM LIST AFTER '('", tokens[cur_token].line_num);
		goto func_failed;
	}
	//prnr from check_params
	if(!func->children.emplace_back(check_Stmnt()))
	{
		report_Error("EXPECTED STMNT AFTER ')'", tokens[cur_token].line_num);
		goto func_failed;
	}
	
	return func;


func_failed:
	report_Error("PARSING OF FUNC FAILED", tokens[start_token].line_num);

	delete func;
	return nullptr;
}

Node* check_RType()
{
	//RTYPE   → "void"
    //        | "int"
	if(check_Next_Token(Token_Class::kint)
	|| check_Next_Token(Token_Class::kvod))
		return new Node(tokens[cur_token]);
	
	report_Error("EXPECTED \"int\" XOR \"void\"", 0);
	return nullptr;
}
Node* check_Params()
{
	//this translation is not 1:1 with grammar 
	//it adds params as children to PARAMS
	//more direct translation would add subtress 
	//PARAMS  → id "," PARAMS
    //    | id
	//    | empty
	Node* params = new Node(Node_Type::prms);
	bool more_than_one = false;
	while(!check_Next_Token(Token_Class::prnr))
	{
		if(more_than_one
		&& !check_Next_Token(Token_Class::coma))
		{
			report_Error("EXPECTED ',' TO SEPARATE PARAMS", tokens[cur_token].line_num);
			delete params;
			return nullptr;
		}
		if(check_Next_Token(Token_Class::enof))
		{
			report_Error("UNEXPECTED END OF FILE", tokens[cur_token].line_num);
			delete params;
			return nullptr;
		}

		if(check_Next_Token(Token_Class::iden))
		{
			params->children.emplace_back(new Node(tokens[cur_token]));
		}
		else
		{
			if(check_Next_Token(Token_Class::coma))
			{
				if(more_than_one)
					report_Error("UNEXPECTED ',', MORE THAN ONE ','", tokens[cur_token].line_num);
				else
					report_Error("UNEXPECTED ',', FIRST TOKN IN PARAM LIST SHOULD BE ID", tokens[cur_token].line_num);
			}
			else
				report_Error("EXPECTED ID AS NAME OF PARAM", tokens[cur_token].line_num);
			delete params;
			return nullptr;
		}

		more_than_one = true;
	}
	return params;
}

Node* check_Stmnt()
{
	Node* stmnt = new Node(Node_Type::stmn);
	
	//STMNT   → "{" STMNTS "}"
	if(check_Next_Token(Token_Class::brkl))
	{
		stmnt->children.emplace_back(check_Stmns());
		return stmnt;
	}
	//check for brkr is performed in stmnts

	//STMNT → "return" EXPR ";"
	if(check_Next_Token(Token_Class::kret))
	{
		stmnt->children.emplace_back(new Node(tokens[cur_token]));

		Node* expr = check_Expr();
		if(nullptr == expr)
		{
			report_Error("EXPECTED EXPR AFTER \"RETURN\"", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}
		stmnt->children.emplace_back(expr);
		
		if(!check_Next_Token(Token_Class::scln))
		{
			report_Error("EXPECTED ';' AFTER RETURN STMNT", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}

		return stmnt;
	}
	
	//STMNT → "var" id "=" EXPR ";"
	if(check_Next_Token(Token_Class::kvar))
	{
		stmnt->children.emplace_back(new Node(tokens[cur_token]));
		
		if(!check_Next_Token(Token_Class::iden))
		{
			report_Error("EXPECTED ID AFTER \"var\"", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}
		stmnt->children.emplace_back(new Node(tokens[cur_token]));
		
		if(!check_Next_Token(Token_Class::oset))
		{
			report_Error("EXPECTED '=' AFTER ID IN VAR STMNT", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}

		Node* expr = check_Expr();
		if(nullptr == expr)
		{
			report_Error("EXPECTED EXPR AFTER '=' IN VAR STMNT", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}
		stmnt->children.emplace_back(expr);
		
		if(!check_Next_Token(Token_Class::scln))
		{
			report_Error("EXPECTED ';' AFTER VAR STMNT", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}

		return stmnt;
	}

	//STMNT → "if" "(" EXPR ")" STMNT
	if(check_Next_Token(Token_Class::kif))
	{
		stmnt->children.emplace_back(new Node(tokens[cur_token]));
		
		if(!check_Next_Token(Token_Class::prnl))
		{
			report_Error("EXPECTED '(' AFTER \"if\"", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}
		
		Node* expr = check_Expr();
		if(nullptr == expr)
		{
			if(check_Next_Token(Token_Class::prnr))
				report_Error("NO EXPR IN IF STMNT", tokens[cur_token].line_num);
			else
				report_Error("INVALID EXPR IN IF STMNT", tokens[cur_token].line_num);
			goto stmnt_fail;
		}
		stmnt->children.emplace_back(expr);

		if(!check_Next_Token(Token_Class::prnr))
		{
			report_Error("EXPECTED ')' AFTER EXPR IN IF STMNT", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}
		Node* child_stmnt  = check_Stmnt();
		if(nullptr == child_stmnt)
		{
			report_Error("INVALID STMNT IN IF STMNT", tokens[cur_token].line_num);
			goto stmnt_fail;
		}
		stmnt->children.emplace_back(child_stmnt);
	
		return stmnt;
	}
	
	//STMNT → ";"
	if(check_Next_Token(Token_Class::scln))
	{
		stmnt->children.emplace_back(new Node(tokens[cur_token]));
		return stmnt;
	}

	//STMNT → EXPR ";"
	if(stmnt->children.emplace_back(check_Expr()))
	{
		if(!check_Next_Token(Token_Class::scln))
		{
			report_Error("EXPECTED ';' AFTER EXPR STMNT", tokens[cur_token].line_num);
	
			goto stmnt_fail;
		}

		return stmnt;
	}

stmnt_fail:
	delete stmnt;
	return nullptr;
}

Node* check_Stmns()
{
	Node* stmnts = new Node(Node_Type::stms);
	
	//this translation is not 1:1 with grammar 
	//it adds stmnts as children to stmnts 
	//more direct translation would add subtress	
	//STMNTS  → STMNT STMNTS
    //        | STMNT
	while(!check_Next_Token(Token_Class::brkr))
	{
		if(check_Next_Token(Token_Class::enof))
		{
			report_Error("UNEXPECTED END OF FILE", tokens[cur_token].line_num);
			delete stmnts;
			return nullptr;
		}

		Node* child = check_Stmnt();
		if(nullptr != child)
		{
			stmnts->children.emplace_back(child);
		}
		else
		{
			report_Error("INVALID STMNT", tokens[cur_token].line_num);
			return stmnts;
		}

	}
	return stmnts;
}

Node* check_Expr()
{
	Node* expr = new Node(Node_Type::expr);
	
	int cur_token_saved = cur_token;
	
	//EXPR    → "op" EXPR operator EXPR
	if(check_Next_Token(Token_Class::kop))
	{
		if(expr->children.emplace_back(check_Expr())
		&& 
		   (  check_Next_Token(Token_Class::omul)
		   || check_Next_Token(Token_Class::odiv)
		   || check_Next_Token(Token_Class::oadd)
		   || check_Next_Token(Token_Class::osub)
		   || check_Next_Token(Token_Class::oequ)
		   || check_Next_Token(Token_Class::oneq)
		   || check_Next_Token(Token_Class::ogrt)
		   || check_Next_Token(Token_Class::oles))
		&& expr->children.emplace_back(new Node(tokens[cur_token]))
		&& expr->children.emplace_back(check_Expr()))
			return expr;
		else
		{
			report_Error("INVALID OPERATOR EXPR", tokens[cur_token].line_num);
			goto expr_fail;
		}

	}
	
	expr->children.clear();
	cur_token = cur_token_saved;

	//EXPR → ( EXPR )
	if(check_Next_Token(Token_Class::prnl)
	&& expr->children.emplace_back(check_Expr())
	&& check_Next_Token(Token_Class::prnr))
		return expr;
	
	expr->children.clear();
	cur_token = cur_token_saved;
	
	//EXPR → id "=" EXPR
	if(check_Next_Token(Token_Class::iden)
	&& expr->children.emplace_back(new Node(tokens[cur_token]))
	&& check_Next_Token(Token_Class::oset)
	&& expr->children.emplace_back(new Node(tokens[cur_token]))
	&& expr->children.emplace_back(check_Expr()))
		return expr;

	expr->children.clear();
	cur_token = cur_token_saved;

	//EXPR → id "(" ARGS ")" 
	if(check_Next_Token(Token_Class::iden)
	&& expr->children.emplace_back(new Node(tokens[cur_token]))
	&& check_Next_Token(Token_Class::prnl)
	&& expr->children.emplace_back(check_Args()))
	//parenr checked by args
		return expr;

	expr->children.clear();
	cur_token = cur_token_saved;
	
	//EXPR → id
	if(check_Next_Token(Token_Class::iden))
	{
		expr->children.emplace_back(new Node(tokens[cur_token]));
		return expr;
	}

	expr->children.clear();
	cur_token = cur_token_saved;

	//EXPR → string
	if(check_Next_Token(Token_Class::strn))
	{
		expr->children.emplace_back(new Node(tokens[cur_token]));
		return expr;
	}
	
	expr->children.clear();
	cur_token = cur_token_saved;

	//EXPR → number
	if(check_Next_Token(Token_Class::numb))
	{
		if(check_Next_Token(Token_Class::oset))
		{
			report_Error("EXPECTED ID ON LHS OF '='", tokens[cur_token].line_num);
			goto expr_fail;
		}

		expr->children.emplace_back(new Node(tokens[cur_token]));
		return expr;
	}
	
	report_Error("INVALID EXPR", tokens[cur_token_saved + 1].line_num);
expr_fail:
	delete expr;
	return nullptr;
}

Node* check_Args()
{
	//this translation is not 1:1 with grammar 
	//it adds args as children to ARGS
	//more direct translation would add subtress	
	//ARGS    → EXPR "," ARGS
	//        | EXPR
	//        | empty
	Node* args = new Node(Node_Type::args);
	bool more_than_one = false;
	while(!check_Next_Token(Token_Class::prnr))
	{
		if(more_than_one
		&& !check_Next_Token(Token_Class::coma))
		{
			report_Error("EXPECTED ',' TO SEPARATE ARGS", tokens[cur_token].line_num);
			delete args;
			return nullptr;
		}
		if(check_Next_Token(Token_Class::enof))
		{
			report_Error("UNEXPECTED END OF FILE", tokens[cur_token].line_num);
			delete args;
			return nullptr;
		}

		Node* child = check_Expr();
		if(nullptr == child)
		{
			
			if(check_Next_Token(Token_Class::coma))
			{
				if(more_than_one)
					report_Error("UNEXPECTED ',', MORE THAN ONE ','", tokens[cur_token].line_num);
				else
					report_Error("UNEXPECTED ',', FIRST TOKN IN ARG LIST SHOULD BE EXPR", tokens[cur_token].line_num);
			}
			else
				report_Error("EXPECTED EXPR AS AN ARG", tokens[cur_token].line_num);
			
			delete args;
			return nullptr;
		}

		args->children.emplace_back(child);
		more_than_one = true;
	}
	return args;
}

bool check_Next_Token(Token_Class tc)
{
	if(tc == tokens[++cur_token].tc)
		return true;

	cur_token--;
	return false;
}

