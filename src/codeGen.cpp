#include <string>
#include <vector>

#include "node.h"

enum class TAC_Op : char{
	

};

struct TAC{
	unsigned int dest;
	unsigned int src1;
	unsigned int src2;
	TAC_Op op;

	Tac(unsigned int n_dest, unsigned int n_src1, unsigned int n_src2, TAC_Op n_TO)
		: dest(n_dest), src1(n_src1), src2(n_src2), op(n_TO){}
};

std::string gen_Func(Node& node);

std::string TAC_Code_Gen(Node& node)
{
	for(auto& children : node)
	{

	}

}
