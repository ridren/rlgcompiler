#ifndef NODE_H
#define NODE_H

#include "token.h"

//requires TOKEN_DEBUG to be 1
#define NODE_DEBUG 1 

enum class Node_Type : char{
	prog,
	func,
	rtyp,
	prms,
	stmn,
	stms,
	expr,
	args,
	tokn
};

struct Node{
	std::vector<Node*> children;
	Token tkn;
	Node_Type nt;
	
	Node();
	Node(Node_Type n_nt);
	Node(Token& n_tkn);
	~Node();
};

#if NODE_DEBUG 
//to allow for ostream operator<< 
#include <iostream>

std::ostream& operator<<(std::ostream& os, Node_Type nt);
extern int  intendation_level;
extern bool last_child;
std::ostream& operator<<(std::ostream& os, Node& node);
#endif

#endif
