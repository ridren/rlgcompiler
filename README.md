# complier for my own language

# LANGUAGE FEATURES:
- functions
- arithmetic operations
- variable declaration
- conditional statements

## code snippet showing most important thngs:
	 func int:factorial(x)
	 {
		if(op x == 0) return 1;
		return op x * factorial(op x - 1);
	 }
	 func int:main()
	 {
		return factorial(3);
	 }
using "op" before arithmetic operations will change but it was more or less necessary for now because i used Recursive Descend parsing

# compiler description
- lexer
	- could be a lot better, by using more of DFA approach, will change
- parser
	- recursive descent parsing
	- could be better by using shift-reduce, will change
- semantic analyzer
	- recursive over entire parse tree
- IR gen
	- Three Address Code
- Optimizations
	- constant and copy propagation
	- constant folding
	- arithmetic simplifications
- asm gen
	- mostly direct translation of TAC
	- done by assigning each register in TAC to register in x86, but before that numbers of regs are reduced, will use graph coloring
	- terribly bad function code generation, will change

# compilation instruction
	clone entire src/ dir
	create out/ dir
	set XXXX_DEBUG in header files to determine what debug do you want
	//if you want to, turn on optimizations in Makefile by uncommenting "-O3"
	run "make compiler" in dir parent to src and out

