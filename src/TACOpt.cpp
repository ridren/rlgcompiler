#include <vector>
#include <string>
#include <algorithm>

#include "tac.h"

#include <iostream>



unsigned int last_checked = 0;

int find_Next(std::vector<TAC_Stmnt>& stmnts);
int find_Next_Func(std::vector<TAC_Stmnt>& stmnts);
int find_Cnst(unsigned int dest);
int find_Reg(unsigned int dest);
void remove_From_Unused_Regs(unsigned int dest);

struct Const_Prop{
	unsigned int dest;
	int cnst;
};

struct Copy_Prop{
	unsigned int dest;
	int reg;
};

std::vector<Const_Prop> consts_to_prop;
bool const_found = false;
std::vector<unsigned int> unused_regs;
std::vector<Copy_Prop> regs_to_prop;
bool reg_found = false;

void const_Propagation(std::vector<TAC_Stmnt>& stmnts, int& range_left, int& range_right);
void copy_Propagation(std::vector<TAC_Stmnt>& stmnts, int& range_left, int& range_right);
void optimize_Arithmetic(std::vector<TAC_Stmnt>& stmnts);
void dead_Reg_Elim(std::vector<TAC_Stmnt>& stmnts, int& range_left, int& range_right);
void reduce_Reg_Nums(std::vector<TAC_Stmnt>& stmnts, int& range_left, int& range_right);

void optimize(std::vector<TAC_Stmnt>& stmnts)
{
	int f_range_left = last_checked;
	int f_range_right = find_Next_Func(stmnts);
	
	int l_range_left = 0;
	int l_range_right = last_checked;
	//dead code elimination over whole function
	while(f_range_left != f_range_right)
	{
		//but const and copy propagation only over smallest blocks
		do
		{
			consts_to_prop.clear();
			regs_to_prop.clear();
			
			last_checked = l_range_right;
			l_range_left = last_checked;
			l_range_right = find_Next(stmnts);
			
			const_Propagation(stmnts, l_range_left, l_range_right);
			copy_Propagation(stmnts, l_range_left, l_range_right);
		
		}
		while(f_range_right != l_range_right);
		dead_Reg_Elim(stmnts, f_range_left, f_range_right);
		reduce_Reg_Nums(stmnts, f_range_left, f_range_right);

		l_range_right = f_range_right;

		//if procedure get optimized to no stmnt, insert nop
		if(f_range_left == f_range_right)
			stmnts.emplace(stmnts.begin() + f_range_left + 1, 0, 0, TAC_REG);
		f_range_right++;

		//ensures that we only work in simple blocks that have no jump in jump out
		//greately simplifies optimization
		last_checked = f_range_right;
		f_range_left = last_checked;
		f_range_right = find_Next_Func(stmnts);
	}

	optimize_Arithmetic(stmnts);

	return;
}

//generally if sees
//	TX = CONST
//	TY = TX
//replaces it with
//	TY = CONST
//and a bit more related to it
void const_Propagation(std::vector<TAC_Stmnt>& stmnts, int& range_left, int& range_right)
{
	for(int i = range_left; i <= range_right; i++)
	{
		TAC_Stmnt& stmnt = stmnts[i];
	
		switch(stmnt.tt)
		{
		case TAC_Type::assig:
		//get consts if assig is in form TX = CONST
			if(-1 == stmnt.src1)
			{
				consts_to_prop.emplace_back(stmnt.dest, stmnt.val1);
				unused_regs.emplace_back(stmnt.dest);
			}
		//replace consts 
			else
			{
				remove_From_Unused_Regs(stmnt.dest);
				int cnst = find_Cnst(stmnt.src1);
				if(const_found)
				{
					//if const found then every register that is uses this register can be removed
					const_found = false;
					stmnt.src1 = -1;
					stmnt.val1 = cnst;
					consts_to_prop.emplace_back(stmnt.dest, stmnt.val1);
					unused_regs.emplace_back(stmnt.dest);
				}
			}
			break;
		case TAC_Type::armul:
		case TAC_Type::ardiv:
		case TAC_Type::aradd:
		case TAC_Type::arsub:
		case TAC_Type::arequ:
		case TAC_Type::arneq:
		case TAC_Type::argrt:
		case TAC_Type::arles:
		{
			bool both_cnst = true;
			remove_From_Unused_Regs(stmnt.dest);
			int cnst = find_Cnst(stmnt.src1);
			if(const_found)
			{
				const_found = false;
				stmnt.src1 = -1;
				stmnt.val1 = cnst;
			} else both_cnst = false;
			cnst = find_Cnst(stmnt.src2);
			if(const_found)
			{
				const_found = false;
				stmnt.src2 = -1;
				stmnt.val2 = cnst;
			}else both_cnst = false;
			
			//if both are consts we can evaluate it at compile time so we have more consts to propagate
			if(both_cnst)
			{
				int res;
				switch(stmnt.tt)
				{
				case TAC_Type::armul:
					res = stmnt.val1 * stmnt.val2;
					break;
				case TAC_Type::ardiv:
					res = stmnt.val1 / stmnt.val2;
					break;
				case TAC_Type::aradd:
					res = stmnt.val1 + stmnt.val2;
					break;
				case TAC_Type::arsub:
					res = stmnt.val1 - stmnt.val2;
					break;
				case TAC_Type::arequ:
					res = (stmnt.val1 == stmnt.val2);
					break;
				case TAC_Type::arneq:
					res = (stmnt.val1 != stmnt.val2);
					break;
				case TAC_Type::argrt:
					res = (stmnt.val1 > stmnt.val2);
					break;
				case TAC_Type::arles:
					res = (stmnt.val1 < stmnt.val2);
					break;
				//shouldnt be possible
				default:
					break;
				}

				stmnt.tt = TAC_Type::assig;
				stmnt.val1 = res;
				consts_to_prop.emplace_back(stmnt.dest, stmnt.val1);
				unused_regs.emplace_back(stmnt.dest);
			}

			break;
		}
		//could be better but for now is enough
		//if sees any reg used in stmnt with funccall then this reg is marked as used
		case TAC_Type::fcall:
			{
				remove_From_Unused_Regs(stmnt.dest);
				for(auto& reg : stmnt.params)
				{
					remove_From_Unused_Regs(reg);
				}
			}
			break;
		case TAC_Type::retur:
		case TAC_Type::ifblk:
			if(-1 != stmnt.src1)
			{
				int cnst = find_Cnst(stmnt.src1);
				if(const_found)
				{
					const_found = false;
					stmnt.val1 = cnst;
					stmnt.src1 = -1;
				}
			}
			break;
		default:
			break;
		}
	}
}

//generally if sees
//	TX = TA
//	TY = TX
//replaces it with
//	TY = TA
//
//if sees
// TX = TA + TB
// TY = TX
//replaces it with
// TY = TA + TB
// TX = TY
//
//most stuff is similar to const propagation, except there is less of it
void copy_Propagation(std::vector<TAC_Stmnt>& stmnts, int& range_left, int& range_right)
{
	for(int i = range_left; i <= range_right; i++)
	{
		TAC_Stmnt& stmnt = stmnts[i];
	
		switch(stmnt.tt)
		{
		case TAC_Type::assig:
		//get 1:1 regs
			if(-1 != stmnt.src1)
			{
				int src1 = find_Reg(stmnt.src1);
				if(reg_found)
				{
					reg_found = false;
					stmnt.src1 = src1;
				}
				remove_From_Unused_Regs(stmnt.src1);
				regs_to_prop.emplace_back(stmnt.dest, stmnt.src1);
				unused_regs.emplace_back(stmnt.dest);
			}
			break;
		case TAC_Type::armul:
		case TAC_Type::ardiv:
		case TAC_Type::aradd:
		case TAC_Type::arsub:
		case TAC_Type::arequ:
		case TAC_Type::arneq:
		case TAC_Type::argrt:
		case TAC_Type::arles:
		{
		//propagate regs in operands of aritmethic expr
			remove_From_Unused_Regs(stmnt.dest);
			int reg;
			if(-1 != stmnt.src1)
			{
				reg = find_Reg(stmnt.src1);
				if(reg_found)
				{
					reg_found = false;
					stmnt.src1 = reg;
				} 
				remove_From_Unused_Regs(stmnt.src1);
			}
			if(-1 != stmnt.src2)
			{
				reg = find_Reg(stmnt.src2);
				if(reg_found)
				{
					reg_found = false;
					stmnt.src2 = reg;
				}
				remove_From_Unused_Regs(stmnt.src2);
			}

			//useful when assigning several times to single var
			//if sees
			// TX = TA + TB
			// TY = TX
			//replaces it with
			// TY = TA + TB
			// TX = TY
			if(TAC_Type::assig == stmnts[i + 1].tt
			&& stmnts[i + 1].src1 == static_cast<int>(stmnts[i].dest))
			{
				unsigned int cur_dest = stmnt.dest;
				stmnt.dest = stmnts[i + 1].dest;
				stmnts[i + 1].dest = cur_dest;
				stmnts[i + 1].src1 = static_cast<unsigned int>(stmnt.dest);
			}

			break;
		}
		
		//could be better but for now is enough
		//if sees any reg used in stmnt with funccall then this reg is marked as used
		case TAC_Type::fcall:
			{
				remove_From_Unused_Regs(stmnt.dest);
				for(auto& reg : stmnt.params)
				{
					remove_From_Unused_Regs(reg);
				}
			}
			break;
		case TAC_Type::retur:
		case TAC_Type::ifblk:
			if(-1 != stmnt.src1)
			{
				int src1 = find_Reg(stmnt.src1);
				if(reg_found)
				{
					reg_found = false;
					stmnt.src1 = src1;
				}
				remove_From_Unused_Regs(stmnt.src1);
			}
			break;
		default:
			break;
		}
	}
}

//generally if expr is known to have some val even if it has not only consts then optimize it
//details in each optimizations
void optimize_Arithmetic(std::vector<TAC_Stmnt>& stmnts)
{
	for(auto& stmnt : stmnts)
	{
		switch(stmnt.tt)
		{
		case TAC_Type::armul:
			//X * 0 = 0
			//0 * X = 0
			if((-1 == stmnt.src1
			&& 0 == stmnt.val1)
			||( -1 == stmnt.src2
			&& 0 == stmnt.val2))
			{
				stmnt.tt = TAC_Type::assig;
				stmnt.src1 = -1;
				stmnt.val1 = 0;
			}
			//1 * X = X
			else if(-1 == stmnt.src1
			&& 1 == stmnt.val1)
			{
				stmnt.tt = TAC_Type::assig;
				stmnt.src1 = stmnt.src2;
			}
			//X * 1 = X
			else if(-1 == stmnt.src2
			&& 1 == stmnt.val2)
			{
				stmnt.tt = TAC_Type::assig;
			}
			break;
		case TAC_Type::ardiv:
			//X / 1 = X
			if(-1 == stmnt.src2
			&& 1 == stmnt.val2)
			{
				stmnt.tt = TAC_Type::assig;
			}
		[[fallthrough]];
		case TAC_Type::arequ:
			//X / X = 1
			//X == X = 1
			if(stmnt.src1 == stmnt.src2
			&& stmnt.src1 != -1)
			{
				stmnt.tt = TAC_Type::assig;
				stmnt.src1 = -1;
				stmnt.val1 = 1;
			}
			break;
		case TAC_Type::aradd:
			//0 + X = X
			if(-1 == stmnt.src1
			&& 0 == stmnt.val1)
			{
				stmnt.tt = TAC_Type::assig;
				stmnt.src1 = stmnt.src2;
				break;
			}
			//X + 0 = X
			else if(-1 == stmnt.src2
			&& 0 == stmnt.val2)
			{
				stmnt.tt = TAC_Type::assig;
			}
			break;
		case TAC_Type::arsub:
			//X - 0 = X
			if(-1 == stmnt.src2
			&& 0 == stmnt.val2)
			{
				stmnt.tt = TAC_Type::assig;
			}
		[[fallthrough]];
		case TAC_Type::arneq:
			//X - X = 0
			//X != X = 0
			if(stmnt.src1 == stmnt.src2
			&& stmnt.src1 != -1)
			{
				stmnt.tt = TAC_Type::assig;
				stmnt.src1 = -1;
				stmnt.val1 = 0;
			}
			break;
		default:
			break;
		}
	}
}

//removes unused regs specified by other funcs 
void dead_Reg_Elim(std::vector<TAC_Stmnt>& stmnts, int& range_left, int& range_right)
{
	for(int i = range_right; i >= range_left; i--)
	{
		if(unused_regs.size() == 0) return;
		
		TAC_Stmnt& stmnt = stmnts[i];
		if(TAC_Type::label == stmnt.tt
		|| TAC_Type::retur == stmnt.tt)
			continue;
		
		if(stmnt.dest == unused_regs.back())
		{
			stmnts.erase(stmnts.begin() + i);
			unused_regs.pop_back();
			range_right--;
		}
	}
	//just in case
	unused_regs.clear();
}

void add_used_reg(std::vector<unsigned int>& used_regs, unsigned int n_reg);
bool find_reg(std::vector<unsigned int>& old_regs, unsigned int reg_to_find, unsigned int& new_loc);

// for example if we use T1, T3, T10, changes it to T0, T1, T3
void reduce_Reg_Nums(std::vector<TAC_Stmnt>& stmnts, int& f_range_left, int& f_range_right)
{
	//registers that are func parameters cannot be touched
	unsigned int do_not_touch_size = stmnts[f_range_left].param_amount;

	std::vector<unsigned int> used_regs;
	std::vector<unsigned int> changed_regs;

	//gatering which register is used
	for(int i = f_range_left; i <= f_range_right; i++)
	{
		TAC_Stmnt& stmnt = stmnts[i];
		switch(stmnt.tt)
		{
		case TAC_Type::assig:
			add_used_reg(used_regs, stmnt.dest);
			if(-1 != stmnt.src1)
			{
				add_used_reg(used_regs, stmnt.src1);
			}
			break;
		case TAC_Type::armul:
		case TAC_Type::ardiv:
		case TAC_Type::aradd:
		case TAC_Type::arsub:
		case TAC_Type::arequ:
		case TAC_Type::arneq:
		case TAC_Type::argrt:
		case TAC_Type::arles:
			add_used_reg(used_regs, stmnt.dest);
			if(-1 != stmnt.src1)
			{
				add_used_reg(used_regs, stmnt.src1);
			}
			if(-1 != stmnt.src2)
			{
				add_used_reg(used_regs, stmnt.src2);
			}
			break;
		case TAC_Type::fcall:
			{
				add_used_reg(used_regs, stmnt.dest);
				for(auto& reg : stmnt.params)
				{
					add_used_reg(used_regs, reg);
				}
			}
			break;
		case TAC_Type::retur:
		case TAC_Type::ifblk:
			if(-1 != stmnt.src1)
			{
				add_used_reg(used_regs, stmnt.src1);
			}
			break;
		default:
			break;
		}
	}

	//assigning new regs coresponding to old regs
	changed_regs.reserve(used_regs.size());
	std::sort(used_regs.begin(), used_regs.end());
	unsigned int last_reg = do_not_touch_size;
	for(auto& reg : used_regs)
	{
		if(reg < do_not_touch_size)
		{
			changed_regs.emplace_back(reg);
			continue;
		}
		changed_regs.emplace_back(last_reg++);
	}
	//changing old regs to new regs
	unsigned int new_loc;
	for(int i = f_range_left; i <= f_range_right; i++)
	{
		TAC_Stmnt& stmnt = stmnts[i];
		switch(stmnt.tt)
		{
		case TAC_Type::assig:
			if(find_reg(used_regs, stmnt.dest, new_loc))
				stmnt.dest = changed_regs[new_loc];

			if(-1 != stmnt.src1)
			{
				if(find_reg(used_regs, stmnt.src1, new_loc))
					stmnt.src1 = changed_regs[new_loc];
			}
			break;
		case TAC_Type::armul:
		case TAC_Type::ardiv:
		case TAC_Type::aradd:
		case TAC_Type::arsub:
		case TAC_Type::arequ:
		case TAC_Type::arneq:
		case TAC_Type::argrt:
		case TAC_Type::arles:
			if(find_reg(used_regs, stmnt.dest, new_loc))
				stmnt.dest = changed_regs[new_loc];
			if(-1 != stmnt.src1)
			{
				if(find_reg(used_regs, stmnt.src1, new_loc))
					stmnt.src1 = changed_regs[new_loc];
			}
			if(-1 != stmnt.src2)
			{
				if(find_reg(used_regs, stmnt.src2, new_loc))
					stmnt.src2 = changed_regs[new_loc];
			}
			break;
		case TAC_Type::fcall:
			if(find_reg(used_regs, stmnt.dest, new_loc))
				stmnt.dest = changed_regs[new_loc];
			for(auto& reg : stmnt.params)
			{
				if(find_reg(used_regs, reg, new_loc))
					reg = changed_regs[new_loc];
			}
			break;
		case TAC_Type::retur:
		case TAC_Type::ifblk:
			if(-1 != stmnt.src1)
			{
				if(find_reg(used_regs, stmnt.src1, new_loc))
					stmnt.src1 = changed_regs[new_loc];
			}
			break;
		default:
			break;
		}
	}
}
void add_used_reg(std::vector<unsigned int>& used_regs, unsigned int n_reg)
{
	for(auto& reg : used_regs)
	{
		if(reg == n_reg) return;
	}
	used_regs.emplace_back(n_reg);
}

bool find_reg(std::vector<unsigned int>& old_regs, unsigned int reg_to_find, unsigned int& new_loc)
{
	for(unsigned int i = 0; i < old_regs.size(); i++)
	{
		auto& reg = old_regs[i];
		if(reg == reg_to_find)
		{
			new_loc = i;
			return true;
		}
		if(reg > reg_to_find)
			return false;
	}
	return false;
}

//finds next label or if to use only smallest blocks in optimization
int find_Next(std::vector<TAC_Stmnt>& stmnts)
{
	if(last_checked >= stmnts.size() - 1) return last_checked;
	
	int i = last_checked;
	while(stmnts[++i].tt != TAC_Type::label
	&&    stmnts[i].tt != TAC_Type::ifblk)
	{
		if(i == static_cast<int>(stmnts.size() - 1)) return i;
	}
	return i;
}
int find_Next_Func(std::vector<TAC_Stmnt>& stmnts)
{
	if(last_checked >= stmnts.size() - 1) return last_checked;
	
	int i = last_checked;
	while(stmnts[++i].tt != TAC_Type::label
	||    stmnts[i].str[0] == 'L') 
	{
		if(i == static_cast<int>(stmnts.size() - 1)) return i;
	}
	return i;
}

int find_Cnst(unsigned int dest)
{
	for(int i = consts_to_prop.size() - 1; i >= 0; i--)
	{
		if(consts_to_prop[i].dest == dest) {const_found = true; return consts_to_prop[i].cnst; }
	}
	return -1;
}
int find_Reg(unsigned int dest)
{
	for(int i = regs_to_prop.size() - 1; i >= 0; i--)
	{
		if(regs_to_prop[i].dest == dest) {reg_found = true; return regs_to_prop[i].reg; }
	}
	return -1;
}

void remove_From_Unused_Regs(unsigned int dest)
{
	for(int i = unused_regs.size() - 1; i >= 0; i--)
	{
		if(unused_regs[i] == dest) {unused_regs.erase(unused_regs.begin() + i);	return;}
	}
}

