#include <string>
#include <vector>

#include "tac.h"

#include <iostream>

int find_Next_Func(std::vector<TAC_Stmnt>& stmnts); //definition in TACOpt.cpp
std::string translate_Func(std::vector<TAC_Stmnt>& stmnts, int range_left, int range_right);

extern unsigned int last_checked;

bool main_exists = false;

std::string x86_Gen(std::vector<TAC_Stmnt>& stmnts)
{
	last_checked = 0;

	int f_range_left = last_checked;
	int f_range_right = find_Next_Func(stmnts);
	
	std::string output;
	
	//translates one func at a time
	while(f_range_left != f_range_right)
	{
		if(stmnts[f_range_left].str == "Fmain")
			main_exists = true;
		
		output += translate_Func(stmnts, f_range_left, f_range_right);

		last_checked = f_range_right;
		f_range_left = last_checked ;
		f_range_right = find_Next_Func(stmnts);
	}

	if(!main_exists)
		output += "Fmain:\n\tmov \trax, 1\n\tret\n";

	return output;
}

std::string get_Reg_From_Num(unsigned int num);

std::string translate_Func(std::vector<TAC_Stmnt>& stmnts, int range_left, int range_right)
{
	std::string output;
	for(int i = range_left; i <= range_right; i++)
	{
		TAC_Stmnt& stmnt = stmnts[i];
	
		switch(stmnt.tt)
		{
		case TAC_Type::label:
			if(i == range_right && stmnt.str[0] != 'L') break;
			output += stmnt.str + ":\n";
			
			//add nop if label is last
			if(i == range_right && stmnt.str[0] == 'L') 
				output += "\tnop\n";

			break;
		case TAC_Type::assig:
		//get consts if assig is in form TX = CONST
			output += "\tmov \t";
			output += get_Reg_From_Num(stmnt.dest) + ", ";
			if(-1 != stmnt.src1)
				output += get_Reg_From_Num(stmnt.src1) + '\n';
			else
				output += std::to_string(stmnt.val1) + '\n';
			break;

		case TAC_Type::armul:
		case TAC_Type::ardiv:
		case TAC_Type::aradd:
		case TAC_Type::arsub:
			if(static_cast<int>(stmnt.dest) == stmnt.src1)
			{
				switch(stmnt.tt)
				{
				case TAC_Type::armul:
					output += "\timul\t";
					break;
				case TAC_Type::ardiv:
					output += "\tidiv\t";
					break;
				case TAC_Type::aradd:
					output += "\tadd \t";
					break;
				case TAC_Type::arsub:
					output += "\tsub \t";
					break;
				default:
					break;
				}
				output += get_Reg_From_Num(stmnt.dest) + ", ";
				if(-1 == stmnt.src2)
					output += std::to_string(stmnt.val2);
				else
					output += get_Reg_From_Num(stmnt.src2);
	
				output += '\n';
				break;
			}
			else
			{
				output += "\tmov \t";
				output += get_Reg_From_Num(stmnt.dest) + ", ";
				
				if(-1 == stmnt.src1)
					output += std::to_string(stmnt.val1);
				else
					output += get_Reg_From_Num(stmnt.src1);
				
				switch(stmnt.tt)
				{
				case TAC_Type::armul:
					output += "\n\timul\t";
					break;
				case TAC_Type::ardiv:
					output += "\n\tidiv\t";
					break;
				case TAC_Type::aradd:
					output += "\n\tadd \t";
					break;
				case TAC_Type::arsub:
					output += "\n\tsub \t";
					break;
				default:
					break;
				}
				
				output += get_Reg_From_Num(stmnt.dest) + ", ";
				
				if(-1 == stmnt.src2)
					output += std::to_string(stmnt.val2);
				else
					output += get_Reg_From_Num(stmnt.src2);
				
				output += '\n';
			}
			break;

		case TAC_Type::arequ:
		case TAC_Type::arneq:
		case TAC_Type::argrt:
		case TAC_Type::arles:
			{
			bool reversed = false;
			output += "\tmov \t";
			output += get_Reg_From_Num(stmnt.dest) + ", 0\n";
			output += "\tcmp \t";
			//both cant be immiediate
			if(-1 == stmnt.src1)
			{
				output += get_Reg_From_Num(stmnt.src2) + ", ";
				output += std::to_string(stmnt.val1) + '\n';
				//because we use args in different order
				reversed = true;
			}
			else if(-1 == stmnt.src2)
			{
				output += get_Reg_From_Num(stmnt.src1) + ", ";
				output += std::to_string(stmnt.val2) + '\n';

			}
			else
			{
				output += get_Reg_From_Num(stmnt.src1) + ", ";
				output += get_Reg_From_Num(stmnt.src2) + '\n';
			}
			//for later cmov 
			output += "\tmov \trax, 1\n";
			switch(stmnt.tt)
			{
			case TAC_Type::arequ:
				output += "\tcmove\t";
				break;
			case TAC_Type::arneq:
				output += "\tcmovne\t";
				break;
			case TAC_Type::argrt:
				if(reversed)
				{
					output += "\tcmovl\t";
				}
				else
				{
					output += "\tcmovg\t";
				}
				break;
			case TAC_Type::arles:
				if(reversed)
				{
					output += "\tcmovg\t";
				}
				else
				{
					output += "\tcmovl\t";
				}
				break;
			default:
				break;
			}
			output += get_Reg_From_Num(stmnt.dest) + ", rax\n";

			break;
			}

	case TAC_Type::fcall:
			{
				output += "\tpush\trdi\n"
				          "\tpush\trbx\n"
				          "\tpush\trsi\n"
				          "\tpush\trdx\n"
				          "\tpush\trcx\n"
				          "\tpush\tr8 \n"
				          "\tpush\tr9 \n"
				          "\tpush\tr10\n"
				          "\tpush\tr11\n"
				          "\tpush\tr12\n"
				          "\tpush\tr13\n"
				          "\tpush\tr14\n"
				          "\tpush\tr15\n";
				for(unsigned int i = 0; i < stmnt.params.size(); i++)
				{
					output += "\tmov \t";
					output += get_Reg_From_Num(i) + ", ";
					output += get_Reg_From_Num(stmnt.params[i]) + '\n';
				}
				output += "\tcall\t";
				output += stmnt.str;
				output += "\n"
				          "\tpop \tr15\n"
				          "\tpop \tr14\n"
				          "\tpop \tr13\n"
				          "\tpop \tr12\n"
				          "\tpop \tr11\n"
				          "\tpop \tr10\n"
				          "\tpop \tr9 \n"
				          "\tpop \tr8 \n"
				          "\tpop \trcx\n"
				          "\tpop \trdx\n"
				          "\tpop \trsi\n"
				          "\tpop \trbx\n"
				          "\tpop \trdi\n"
						  "\tmov \t";
				output += get_Reg_From_Num(stmnt.dest) + ", rax\n";
			}

			break;

		case TAC_Type::retur:
			output += "\tmov \trax, ";
			if(-1 == stmnt.src1)
				output += std::to_string(stmnt.val1) + '\n';
			else
				output += get_Reg_From_Num(stmnt.src1) + '\n';

			output += "\tret\n";
			break;

		case TAC_Type::ifblk:
			if(-1 != stmnt.src1)
			{
				std::string name = get_Reg_From_Num(stmnt.src1);
				output += "\ttest\t";
				output += name + ", ";
				output += name + '\n';
			}
			//remove later, const vals will not appear here
			else
			{
				output += "\tmov \trax, ";
				output += std::to_string(stmnt.val1);
				output += "\n\ttest\trax, rax\n";
			}
			output += "\tjz  \t";
			output += stmnt.str + '\n';
			break;
	
		default:
			break;
		}
	}
	return output;
}
std::string get_Reg_From_Num(unsigned int num)
{
	switch(num)
	{
	case  0:
		return "rdi";
	case  1:
		return "rsi";
	case  2:
		return "rdx";
	case  3:
		return "rcx";
	case  4:
		return "r8 ";
	case  5:
		return "r9 ";
	case  6:
		return "rbx";
	case  7:
		return "r10";
	case  8:
		return "r11";
	case  9:
		return "r12";
	case 10:
		return "r13";
	case 11:
		return "r14";
	case 12:
		return "r15";
	}

	return "";
}
