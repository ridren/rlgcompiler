#ifndef TOKEN_H
#define TOKEN_H

#define TOKEN_DEBUG 1 

enum class Token_Class : char{
	kfnc,
	kret,
	kvar,
	kint,
	kvod,
	kif ,
	kop ,
	coma,
	coln,
	scln,
	prnl,
	prnr,
	brkl,
	brkr,
	numb,
	strn,
	iden,
	omul,
	odiv,
	oadd,
	osub,
	oset,
	oequ,
	oneq,
	ogrt,
	oles,
	enof
};

#if TOKEN_DEBUG
//to allow for ostream operator<<
#include <iostream>

std::ostream& operator<<(std::ostream& os, Token_Class tc);
#endif 

struct Token{
	std::string str;
	unsigned int line_num = 0;
	Token_Class tc;
	
	Token();
	Token(Token_Class n_tc, const std::string& n_str, unsigned int n_line);
	Token(Token_Class n_tc, char n_chr, unsigned int n_line);
};

#endif
