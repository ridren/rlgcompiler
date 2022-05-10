RED := '\033[0;31m'
YELLOW := '\033[0;33m'
NC := '\033[0m' #no color

files := out/main.o out/lexer.o out/parser.o out/semAnal.o out/TACGen.o out/TACOpt.o out/x86Gen.o 
compile_args = #-O3

#default
default:
	@echo -e no option specifed

#recompile everything
recompile: refresh compiler 

refresh:
	@touch src/*

clear:
	@clear

#linking
compiler: clear ${files}
	@echo -e ${RED}linking program${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic -o comp ${files}

#compiling
out/main.o: src/main.cpp
	@echo -e ${YELLOW}compiling main.cpp${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic ${compile_args} -c src/main.cpp
	@mv main.o out/

out/lexer.o: src/lexer.cpp
	@echo -e ${YELLOW}compiling lexer.cpp${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic ${compile_args} -c src/lexer.cpp
	@mv lexer.o out/

out/parser.o: src/parser.cpp
	@echo -e ${YELLOW}compiling parser.cpp${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic ${compile_args} -c src/parser.cpp
	@mv parser.o out/

out/semAnal.o: src/semAnal.cpp
	@echo -e ${YELLOW}compiling semAnal.cpp${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic ${compile_args} -c src/semAnal.cpp
	@mv semAnal.o out/

out/TACGen.o: src/TACGen.cpp
	@echo -e ${YELLOW}compiling TACGen.cpp${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic ${compile_args} -c src/TACGen.cpp
	@mv TACGen.o out/

out/TACOpt.o: src/TACOpt.cpp
	@echo -e ${YELLOW}compiling TACOpt.cpp${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic ${compile_args} -c src/TACOpt.cpp
	@mv TACOpt.o out/

out/x86Gen.o: src/x86Gen.cpp
	@echo -e ${YELLOW}compiling x86Gen.cpp${NC}
	@g++ -std=c++20 -Wall -Wextra -pedantic ${compile_args} -c src/x86Gen.cpp
	@mv x86Gen.o out/


#updating files
src/main.cpp:
	@touch src/main.cpp

src/lexer.cpp:
	@touch src/lexer.cpp

src/parser.cpp:
	@touch src/parser.cpp

src/semAnal.cpp:
	@touch src/semAnal.cpp

src/TACGen.cpp:
	@touch src/TACGen.cpp

src/TACOpt.cpp:
	@touch src/TACOpt.cpp

src/x86Gen.cpp:
	@touch src/x86Gen.cpp




