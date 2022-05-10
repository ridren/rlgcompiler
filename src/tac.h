#ifndef TAC_H
#define TAC_H

#include <vector>
#include <string>

#define TAC_DEBUG 1

#define TAC_REG 1
#define TAC_CNS 0

enum class TAC_Type : char{
	label,
	fcall,
	assig,
	retur,
	ifblk,
	asstr,
	armul,
	ardiv,
	aradd,
	arsub,
	arequ,
	arneq,
	argrt,
	arles
};

//if srcX is -1 then val is taken from valX
struct TAC_Stmnt{
	std::string str;
	unsigned int dest;
	int src1;
	int src2;
	int val1;
	int val2;
	unsigned int param_amount = 0; //for later optimizations
	TAC_Type tt;

	std::vector<unsigned int> params;

	TAC_Stmnt();
	//labels
	TAC_Stmnt(const std::string& n_label);
	//funccall
	TAC_Stmnt(unsigned int n_dest, const std::string& n_name, const std::vector<unsigned int>& n_params);
	//return
	TAC_Stmnt(int src, bool is_reg);
	//if
	TAC_Stmnt(const std::string& n_label, int src, bool is_reg);
	//assignment
	TAC_Stmnt(unsigned int n_dest, int src, bool is_reg);
	//assignment of string
	TAC_Stmnt(unsigned int n_dest, const std::string& n_str);
	//arithmetic
	TAC_Stmnt(unsigned int n_dest, int n_src1, bool is_reg1, int n_src2, bool is_reg2, TAC_Type ntt);
};

#if TAC_DEBUG
std::ostream& operator<<(std::ostream& os, TAC_Stmnt& ts);
#endif

#endif
