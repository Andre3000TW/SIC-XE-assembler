#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace std;

bool g_SIC = false;

struct TokenType {
	string token;    // what token
	int token_type;  // in which table
	int token_value; // in which entry of the table
}; // end struct TokenType

struct OpType {
	string format;
	string instr;
	string opcode;
}; // end struct OpType

struct SymbolType {
	int addr;
	int type;
	string code;
	string symbol;
}; // end struct SymbolType

int no( int num ) { // convert num to index
	return num - 1;
} // end no()

class Hash {
private:
	int table_size_;
public:
	Hash() {
		table_size_ = 100;
	} // end Hash constructor

	int hash( vector<string> table, string key ) { // return index(hash value)
		int hash_value = 0;
		
		for ( int i = 0 ; i < key.size() ; i++ )
			hash_value += key[i];
		hash_value %= table_size_;
		
		if ( ! table[hash_value].empty() ) // collision occurred
			hash_value = linearProbing( table, hash_value ); 
		
		return hash_value;
	} // end hash()
	
	int linearProbing( vector<string> table, int index ) { // return the new location
		while ( ! table[index].empty() ) {
			if ( ++index == table_size_ )
				index %= table_size_;
		} // end while
		
		return index;
	} // end linearProbing()
	
} ; // end class Hash

class LA {
private:
	Hash hash;
	bool isString_;
	bool isInteger_;
	vector<string>  input_prog_;
	vector< vector<string> > token_table_;
	vector< vector<TokenType> > output_token_;
	
	void tokenTableInit() {
		// initialize the token table
		token_table_.clear(); // initial set up
		fstream inFile;
		string file_name;
		vector<string> buf;
		int table_size = 7;
		int hash_table_size = 100;
		for ( int i = 0 ; i < table_size ; i++ ) { 
			if ( i < 4 ) { // establish table 1-4
				char cstr[5];
				sprintf( cstr, "%d", i + 1 );
				file_name = "Table" + string(cstr) + ".table";
				inFile.open( file_name.c_str(), fstream::in );
				if ( inFile.is_open() ) { // read successfully
					char cstr[255];
					while( inFile.getline( cstr, 255, '\n' ) )
						buf.push_back( cstr );
				
					token_table_.push_back( buf );
					inFile.close();
					buf.clear();
				} // if
				else cout << endl << "error occurred when establish table " << i + 1 << endl;
			} // if
			else { // establish table 5-7
				buf.resize( hash_table_size );
				token_table_.push_back( buf );
			} // else
		} // end for
	} // end tokenTableInit()
	
	void searchInTables( int & no_of_table, string token, int & index_in_table ) {
		// determine which table the token belong to
		// table 1, 2, 3 are not case sensitive
		// if table 5,6,7 => if not exist already => create a new entry in each corresponding table
		if ( inTable( no(1), token, index_in_table ) ) no_of_table = 1;
		else if ( inTable( no(2), token, index_in_table ) ) no_of_table = 2;
		else if ( inTable( no(3), token, index_in_table ) ) no_of_table = 3;
		else if ( inTable( no(4), token, index_in_table ) ) no_of_table = 4;
		else if ( isString_ ) { // string
			if ( ! ( inTable( no(7), token, index_in_table ) ) ) { // token in table already
				index_in_table = hash.hash( token_table_[no(7)], token );
				token_table_[no(7)][index_in_table] = token;
			} // if
			no_of_table = 7;
			isString_ = false;
		} // else if
		else if ( isInteger_ || isDecimal( token ) ) { // integer( X'F1' or 4096 )
			if ( ! ( inTable( no(6), token, index_in_table ) ) ) { // token in table already
				index_in_table = hash.hash( token_table_[no(6)], token );
				token_table_[no(6)][index_in_table] = token;
			} // if
			no_of_table = 6;
			isInteger_ = false;
		} // else if
		else { // symbol
			if ( ! ( inTable( no(5), token, index_in_table ) ) ) { // token in table already
				index_in_table = hash.hash( token_table_[no(5)], token );
				token_table_[no(5)][index_in_table] = token;
			} // if
			no_of_table = 5;
		} // else
	} // end searchInTables()
	
	bool inTable( int no_of_table, string token, int & index_in_table ) {
		// search token in table x
		index_in_table = 0;
		for ( ; index_in_table < token_table_[no_of_table].size() ; index_in_table++ ) {
			if ( no_of_table <= no(3) ) { // case-insensitive
				if ( stricmp( token.c_str(), token_table_[no_of_table][index_in_table].c_str() ) == 0 ) 
					return true;
			} // if
			else if ( token == token_table_[no_of_table][index_in_table] ) return true;
		} // end for
		
		return false;
	} // end inTable()
	
	bool isDecimal( string token ) {
		for ( int i = 0 ; i < token.size() ; i++ )
			if ( ! ( '0' <= token[i] && token[i] <= '9' ) ) return false;
		
		return true;
	} // end isInteger()
	
	bool isWhitespace( char ch ) {
		// space or enter or tab
		return ( ( ch == ' ' ) || ( ch == '\n' ) || ( ch == '\t' ) );
	} // end isWhitespace()
	
	bool isDelimiter( char ch ) {
		int x = 0;
		string find;
		find += ch;
		return inTable( no(4), find, x );
	} // end isDelimiter()
public:
	LA( vector<string> input_prog ) {
		isString_ = false;
		isInteger_ = false;
		output_token_.clear();
		token_table_.clear();
		tokenTableInit();
		input_prog_ = input_prog;
	} // end LA constructor
 
	vector< vector<TokenType> > getToken() {
		// get a token when encounter whitespace or delimiter
		for ( int i = 0 ; i < input_prog_.size() ; i++ ) // append enter to each line
			input_prog_[i] += '\n';
		
		string token;
		TokenType tmp;
		vector<TokenType> buf;
		int no_of_table = 0, index_in_table = 0;
		for ( int i = 0 ; i < input_prog_.size() ; i++ ) {
			buf.clear();
			token.clear();
			for ( int j = 0 ; j < input_prog_[i].size() ; j++ ) {
				if ( input_prog_[i][j] == '.' ) { // comments
					if ( ! token.empty() ) {
						// token 1
						searchInTables( no_of_table, token, index_in_table ); // possible: table 1-6
					
						tmp.token = token;
						tmp.token_type = no_of_table;
						tmp.token_value = index_in_table;
						buf.push_back( tmp );
						
						token.clear();
					} // if
					// token 2 "."
					token += input_prog_[i][j++];
					searchInTables( no_of_table, token, index_in_table );
					
					do { // read the rest part of line
						if ( input_prog_[i][j] != '\n' ) token += input_prog_[i][j];
					} while ( ++j < input_prog_[i].size() ) ;
					
					tmp.token = token;
					tmp.token_type = no_of_table;
					tmp.token_value = index_in_table;
					//buf.push_back( tmp );
					
					token.clear();
				} // if
				else if ( token == "X" && input_prog_[i][j] == '\'' ) {
					isInteger_ = true;
					token.clear();
					token += input_prog_[i][j];
					
					searchInTables( no_of_table, token, index_in_table ); // '\''
					
					tmp.token = token;
					tmp.token_type = no_of_table;
					tmp.token_value = index_in_table;
					buf.push_back( tmp );
					
					j++;
					token.clear();
				} // else if X'F1'
				else if ( token == "C" && input_prog_[i][j] == '\'' ) {
					isString_ = true;
					token.clear();
					token += input_prog_[i][j];
					
					searchInTables( no_of_table, token, index_in_table ); // '\''
					
					tmp.token = token;
					tmp.token_type = no_of_table;
					tmp.token_value = index_in_table;
					buf.push_back( tmp );
					
					j++;
					token.clear();
				} // else if C'EOF'
				else if ( isWhitespace( input_prog_[i][j] ) && ! token.empty() ) { // 1 token
					searchInTables( no_of_table, token, index_in_table ); // possible: table 1-6
					
					tmp.token = token;
					tmp.token_type = no_of_table;
					tmp.token_value = index_in_table;
					buf.push_back( tmp );
					
					token.clear();
				} // if whitespace
				else if ( isDelimiter( input_prog_[i][j] ) ) { // 1 v 2 token
					if ( ! token.empty() ) {
						// token 1
						searchInTables( no_of_table, token, index_in_table ); // possible: table 1-6
					
						tmp.token = token;
						tmp.token_type = no_of_table;
						tmp.token_value = index_in_table;
						buf.push_back( tmp );
						
						token.clear();
					} // if
					// token 2
					token += input_prog_[i][j]; // delimiter
					searchInTables( no_of_table, token, index_in_table );
					
					tmp.token = token;
					tmp.token_type = no_of_table;
					tmp.token_value = index_in_table;
					buf.push_back( tmp );
					
					j++;
					token.clear();
				} // else if delimiter
				
				if ( ! isWhitespace( input_prog_[i][j] ) ) token += input_prog_[i][j];
			} // end for
			
			output_token_.push_back( buf );
		} // end for

		return output_token_;
	} // end getToken()
	
	vector< vector<string> > getTokenTable() {
		return token_table_;
	} // end getTokenTable()
	
}; // end class LA

class CA { // cross-assembler
private:
	int base_reg_;
	int location_counter_;
	vector<OpType> op_table_;
	vector<SymbolType> ltr_tab_;
	vector<SymbolType> ltr_table_;
	vector<SymbolType> symbol_table_;
	
	vector<string> input_prog_;
	vector< vector<string> > token_table_;
	vector< vector<TokenType> > output_token_;
	
	void optableInit() {
		op_table_.clear();
		string instr_set = "ADD,3,18,ADDF,3,58,ADDR,2,90,AND,3,40,CLEAR,2,B4,COMP,3,28,COMPF,3,88,COMPR,2,A0,\
DIV,3,24,DIVF,3,64,DIVR,2,9C,FIX,1,C4,FLOAT,1,C0,HIO,1,F4,J,3,3C,JEQ,3,30,JGT,3,34,\
JLT,3,38,JSUB,3,48,LDA,3,00,LDB,3,68,LDCH,3,50,LDF,3,70,LDL,3,08,LDS,3,6C,LDT,3,74,\
LDX,3,04,LPS,3,D0,MUL,3,20,MULF,3,60,MULR,2,98,NORM,1,C8,OR,3,44,RD,3,D8,RMO,2,AC,\
RSUB,3,4C,SHIFTL,2,A4,SHIFTR,2,A8,SIO,1,F0,SSK,3,EC,STA,3,0C,STB,3,78,STCH,3,54,STF,3,80,\
STI,3,D4,STL,3,14,STS,3,7C,STSW,3,E8,STT,3,84,STX,3,10,SUB,3,1C,SUBF,3,5C,SUBR,2,94,\
SVC,2,B0,TD,3,E0,TIO,1,F8,TIX,3,2C,TIXR,2,B8,WD,3,DC,";

		OpType tmp;
		string buf;
		int count = 1, pos = 0, pre = 0;
		while ( pre < instr_set.size() ) {
			pos = instr_set.find_first_of( ',', pre );
			buf = instr_set.substr( pre, pos - pre );
			switch ( count++ ) {
				case 1:tmp.instr = buf;  // instruction
					   break;
				case 2:tmp.format = buf; // format 1-4
					   break;
				case 3:tmp.opcode = buf; // opcode
					   op_table_.push_back( tmp );
					   count = 1;
					   break;
				default:break;
			} // end switch
			
			pre = pos + 1;
		} // end while
	} // end optableInit()
	
	bool isSymbol( int line, int & index ) {
		if ( index < output_token_[line].size() && output_token_[line][index].token_type == 5 ) {
			index++;
			return true;
		} // if
		
		return false;
	} // end isSymbol()
	
	bool isInstr( int line, int & index ) {
		if ( index < output_token_[line].size() && 
			 ( output_token_[line][index].token_type == 1 || output_token_[line][index].token_type == 2 ) ) {
			index++;
			return true;
		} // if
		
		return false;
	} // end isInstr()
	
	bool isDelimiter( int line, int & index ) {
		if ( index < output_token_[line].size() && output_token_[line][index].token_type == 4 ) {
			index++;
			return true;
		} // if
		
		return false;
	} // end isDelimiter()
	
	bool isOperand( int line, int & index ) {
		if ( index < output_token_[line].size() && 
				 ( output_token_[line][index].token_type == 3 || output_token_[line][index].token_type == 5 ||
				 output_token_[line][index].token_type == 6 || output_token_[line][index].token_type == 7 ) ) {
			index++;
			return true;
		} // if
		
		return false;
	} // end isOperand()
	
	bool isEqualSign( int line, int & index ) {
		if ( index < output_token_[line].size() && output_token_[line][index].token_type == 4 
			&& output_token_[line][index].token_value == 10 ) {
			index++;
			return true;
		} // if
		
		return false;
	} // end isOperand()
	
	bool isEnd( int line, int index ) {
		return index >= output_token_[line].size();
	} // end isEnd()
	
	int isSymOrInt( int line, int index ) {
		if ( index == 0 ) return -1;
		
		if ( output_token_[line][index].token_type == 5 ) return 1;
		else if ( output_token_[line][index].token_type == 6 || output_token_[line][index].token_type == 7 ) return 2;
		else return -1;
	} // end isSymOrInt()
	
	bool reset( int & index, int value ) {
		index = value;
		return true;
	} // end reset()
	
	int inSymboyTable( string symbol ) {
		for ( int i = 0 ; i < symbol_table_.size() ; i++ ) {
			if ( symbol == symbol_table_[i].symbol ) return i; // duplicated
		} // end for
		
		return -1;
	} // end inSymboyTable()
	
	int inLtrTable( string ltr ) {
		for ( int i = 0 ; i < ltr_table_.size() ; i++ ) {
			if ( ltr == ltr_table_[i].symbol ) return i; // duplicated
		} // end for
		
		return -1;
	} // end inLtrTable()
	
	int inLtrTab( string ltr ) {
		for ( int i = 0 ; i < ltr_tab_.size() ; i++ ) {
			if ( ltr == ltr_tab_[i].symbol ) return i; // duplicated
		} // end for
		
		return -1;
	} // end inLtrTable()
	
	int whichFormat( string instr ) { // return format else return bytes
		for ( int i = 0 ; i < op_table_.size() ; i++ ) {
			if ( stricmp( instr.c_str(), op_table_[i].instr.c_str() ) == 0 )
				return strtol( op_table_[i].format.c_str(), NULL, 16);
		} // end for
		
		return 0;
	} // end whichFormat()
	
	int fetchOp( int line, int index ) {
		for ( ; index < output_token_.size() ; index++ )
			if ( isOperand( line, index ) ) return index - 1;
		
		return 0;
	} // end fetchOp()
	
	string getOpcode( string instr ) {
		for ( int i = 0 ; i < op_table_.size() ; i++ ) {
			if ( stricmp( instr.c_str(), op_table_[i].instr.c_str() ) == 0 )
				return op_table_[i].opcode;
		} // end for
		
		return "fuck";
	} // end getOpcode()
	
	string hasLTR( int line, int index ) {
		string ltr;
		ltr.clear();
		for ( ; index < input_prog_[line].size() ; index++ ) {
			if ( input_prog_[line][index] == '=' ) {
				for ( int i = index ; i < input_prog_[line].size() ; i++ )
					ltr += input_prog_[line][i];
				return ltr;
			} // if
		} // end for
		
		return "";
	} // end hasLTR()
	
	bool over3Bytes( int disp ) {
		int count = 0, double_f = 0;
		char buf[33];
		itoa( disp, buf, 16 ); // convert int to str
		string buf_disp = buf;
		
		for ( int i = buf_disp.size() - 1 ; i > 0 ; i-- ) { // stop when encounter double 'f'
			count++;
			if ( double_f == 0 && buf_disp[i] == 'f' ) double_f++;
			else if ( double_f == 1 && buf_disp[i] == 'f' ) { // double 'f'
				count -= 2;
				if ( count > 3 ) return true;
				else return false;
			} // else if
			else double_f = 0;
		} // end for
		
		if ( count > 3 ) return true;
		else return false;
	} // end over3Bytes()
	
	string binToHex( string bin ) {
		string tmp, hex;
		tmp.clear();
		hex.clear();
		for ( int i = 0 ; i < bin.size() ; i++ ) {
			tmp += bin[i];
			if ( ( i + 1 ) % 4 == 0 ) { 
				if ( tmp == "0000" ) hex += "0";
				else if ( tmp == "0001" ) hex += "1";
				else if ( tmp == "0010" ) hex += "2";
				else if ( tmp == "0011" ) hex += "3";
				else if ( tmp == "0100" ) hex += "4";
				else if ( tmp == "0101" ) hex += "5";
				else if ( tmp == "0110" ) hex += "6";
				else if ( tmp == "0111" ) hex += "7";
				else if ( tmp == "1000" ) hex += "8";
				else if ( tmp == "1001" ) hex += "9";
				else if ( tmp == "1010" ) hex += "A";
				else if ( tmp == "1011" ) hex += "B";
				else if ( tmp == "1100" ) hex += "C";
				else if ( tmp == "1101" ) hex += "D";
				else if ( tmp == "1110" ) hex += "E";
				else if ( tmp == "1111" ) hex += "F";	
				tmp.clear();
			} // if
			
		} // end for
		
		return hex;
	} // end binToHex()
	
	string opToBin( string opcode ) {
		string bin;
		bin.clear();
		if ( opcode[0] == '0' ) bin += "0000";
		else if ( opcode[0] == '1' ) bin += "0001";
		else if ( opcode[0] == '2' ) bin += "0010";
		else if ( opcode[0] == '3' ) bin += "0011";
		else if ( opcode[0] == '4' ) bin += "0100";
		else if ( opcode[0] == '5' ) bin += "0101";
		else if ( opcode[0] == '6' ) bin += "0110";
		else if ( opcode[0] == '7' ) bin += "0111";
		else if ( opcode[0] == '8' ) bin += "1000";
		else if ( opcode[0] == '9' ) bin += "1001";
		else if ( opcode[0] == 'A' ) bin += "1010";
		else if ( opcode[0] == 'B' ) bin += "1011";
		else if ( opcode[0] == 'C' ) bin += "1100";
		else if ( opcode[0] == 'D' ) bin += "1101";
		else if ( opcode[0] == 'E' ) bin += "1110";
		else if ( opcode[0] == 'F' ) bin += "1111";
		
		if ( opcode[1] == '0' ) bin += "00";
		else if ( opcode[1] == '1' ) bin += "00";
		else if ( opcode[1] == '2' ) bin += "00";
		else if ( opcode[1] == '3' ) bin += "00";
		else if ( opcode[1] == '4' ) bin += "01";
		else if ( opcode[1] == '5' ) bin += "01";
		else if ( opcode[1] == '6' ) bin += "01";
		else if ( opcode[1] == '7' ) bin += "01";
		else if ( opcode[1] == '8' ) bin += "10";
		else if ( opcode[1] == '9' ) bin += "10";
		else if ( opcode[1] == 'A' ) bin += "10";
		else if ( opcode[1] == 'B' ) bin += "10";
		else if ( opcode[1] == 'C' ) bin += "11";
		else if ( opcode[1] == 'D' ) bin += "11";
		else if ( opcode[1] == 'E' ) bin += "11";
		else if ( opcode[1] == 'F' ) bin += "11";
		
		return bin;
	} // end opToBin()
	
	void lexicalAnalysis() {
		LA lexical_analyzer( input_prog_ );
		output_token_ = lexical_analyzer.getToken();
		token_table_ = lexical_analyzer.getTokenTable();
	} // end lexicalAnalysis()

	int syntaxAnalysis( int line ) {
		// ( label, opcode, operand)
		int index = 0;
		if ( isSymbol( line, index ) ) {
			if ( isDelimiter( line, index ) && isInstr( line, index ) ) return 4;
			else if ( reset(index, 1) && isInstr( line, index ) ) { // symbol-instr~~ (store symbol)
				if ( isEqualSign( line, index ) ) return 1;
				else if ( isDelimiter( line, index ) ) { // symbol-instr-deli~~
					if ( isEnd( line, index ) ) return 1; // symbol-instr-deli
					else if ( isOperand( line, index ) ) { // symbol-instr-deli-op~~
						if ( isEnd( line, index ) ) return 1; // symbol-instr-deli-op
						else if ( reset(index, 4) && isDelimiter( line, index ) && isEnd( line, index ) ) 
							return 1; // symbol-instr-deli-op-deli(string)
						else if ( reset(index, 4) && isDelimiter( line, index ) && isOperand( line, index ) ) 
							return 1; // symbol-instr-deli-op-deli-op
					} // else if
				} // if
				else if ( reset(index, 2) && isOperand( line, index ) ) { // symbol-instr-op~~
					if ( isEnd( line, index ) ) return 1; // symbol-instr-op
					else if ( isDelimiter( line, index ) && isOperand( line, index ) ) 
						return 1;// symbol-instr-op-deli-op
				} // else if
				else if ( reset(index, 2) && isEnd( line, index ) ) return 1; // symbol-instr
			} // if
			
		} // if
		else if ( reset(index, 0) && isInstr( line, index ) ) {
			if ( isEqualSign( line, index ) ) return 2;
			else if ( isDelimiter( line, index ) ) { // instr-deli~~
				if ( isEnd( line, index ) ) return 2; // instr-deli
				else if ( isOperand( line, index ) ) { // instr-deli-op~~
					if ( isEnd( line, index ) ) return 2; // instr-deli-op
					else if ( reset(index, 3) && isDelimiter( line, index ) && isEnd( line, index ) ) 
						return 2; // instr-deli-op-deli(string)
					else if ( reset(index, 3) && isDelimiter( line, index ) && isOperand( line, index ) ) 
						return 2; // instr-deli-op-deli-op
				} // else if
			} // if
			else if ( reset(index, 1) && isOperand( line, index ) ) { // instr-op~~
				if ( isEnd( line, index ) ) return 2; // instr-op
				else if ( isDelimiter( line, index ) && isOperand( line, index ) ) 
					return 2;// instr-op-deli-op
			} // else if
			else if ( reset(index, 1) && isEnd( line, index ) ) return 2; // instr
		} // else if
		else if ( reset(index, 0) && isDelimiter( line, index ) ) { // "+" format 4
			if ( isInstr( line, index ) ) return 3;
		} // else if
		
		else if ( output_token_[line].size() == 0 ) return 5;
		
		return 0;
	} // end syntaxAnalysis()
	
public:
	CA() {
		base_reg_ = 0;
		optableInit();
		ltr_tab_.clear();
		ltr_table_.clear();
		symbol_table_.clear();
		location_counter_ = 0;
		input_prog_.clear();
		token_table_.clear();
		output_token_.clear();
	} // end CA constructor

	bool inputFile( string file_name ) {
		input_prog_.clear(); // initial set up
    	fstream inFile;
        file_name = file_name + ".txt";
        inFile.open( file_name.c_str(), fstream::in );
        if ( inFile.is_open() ) { // read successfully
			char buf[255];
        	while( inFile.getline( buf, 255, '\n' ) )
				input_prog_.push_back( buf );
			
            inFile.close();
            return true;
        } // if
        else { // error
            cout << endl << "### " << file_name << " does not exist! ###" << endl;
            return false;
        } // else
	} // end inputFile()

	void pass1() { // duplicated 
		lexicalAnalysis();
		
		int index = 0, result = 0, tmp_loc = 0;
		string symbol, instr, op, ltr;
		SymbolType tmp;
		for ( int i = 0 ; i < output_token_.size() ; i++ ) {
			ltr.clear();
			result = syntaxAnalysis( i );
			if ( result == 1 ) {
				symbol = token_table_[output_token_[i][0].token_type-1][output_token_[i][0].token_value];
				instr = token_table_[output_token_[i][1].token_type-1][output_token_[i][1].token_value];
				if ( stricmp( instr.c_str(), "start" ) == 0 ) {
					bool minus = false;
					int addr1 = 0, addr2 = 0, sym_index = 0;
					index = fetchOp( i, 2 ); // fetch op1(suppose that there is always a op1)
					if ( isSymOrInt( i, index ) == 1 ) { // symbol
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						sym_index = inSymboyTable( op );
						addr1 = symbol_table_[sym_index].addr;
					} // if
					else if ( isSymOrInt( i, index ) == 2 ) { // int
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						addr1 = strtol(op.c_str(), NULL, 16);
					} // else if
					else if ( index == 0 ) addr1 = location_counter_;
					else addr1 = 0;
					
					index = fetchOp( i, index + 1 ); // fetch op2
					if ( index != 0 ) { // there is op2
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr2 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr2 = strtol(op.c_str(), NULL, 16);
						} // else if
						else addr2 = 0;
						
						if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
							minus = true;
					} // if
					
					if ( minus ) location_counter_ = addr1 - addr2;
					else location_counter_ = addr1 + addr2;
				} // if
				else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
					tmp_loc = location_counter_;
					bool minus = false;
					int addr1 = 0, addr2 = 0, sym_index = 0;
					index = fetchOp( i, 2 ); // fetch op1(suppose that there is always a op1)
					if ( isSymOrInt( i, index ) == 1 ) { // symbol
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						sym_index = inSymboyTable( op );
						addr1 = symbol_table_[sym_index].addr;
					} // if
					else if ( isSymOrInt( i, index ) == 2 ) { // int
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						addr1 = strtol(op.c_str(), NULL, 10);
					} // else if
					else if ( index == 0 ) addr1 = location_counter_;
					else addr1 = 0;
					
					index = fetchOp( i, index + 1 ); // fetch op2
					if ( index != 0 ) { // there is op2
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr2 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr2 = strtol(op.c_str(), NULL, 10);
						} // else if
						else addr2 = 0;
						
						if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
							minus = true;
					} // if
					
					if ( minus ) location_counter_ = addr1 - addr2;
					else location_counter_ = addr1 + addr2;
				} // else if
				
				tmp.addr = location_counter_;
				tmp.symbol = symbol;
				if ( inSymboyTable( tmp.symbol ) != -1 ) cout << "duplicated definition in line " << 5*(i+1) << " !\n";
				else symbol_table_.push_back( tmp );
				
				ltr = hasLTR( i, 2 );
				if ( ! ltr.empty() && inLtrTab( ltr ) == -1 ) { // hasLTR && not in ltr table
					string code;
					index = fetchOp( i, 2 ); // fetch first op
					if ( output_token_[i][index].token_type == 6 ) { // X'F1'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						code += op;
					} // if
					else { // C'EOF'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) 
							code.append("00");
							
						char buf[20];
						for ( int i = 0 ; i < op.size() ; i++ ) {
							sprintf( buf, "%X", int(op[i]));
							code += buf;
						} // end for
					} // else
						
					tmp.type = output_token_[i][index].token_type;
					tmp.code = code;
					tmp.symbol = ltr;
					ltr_tab_.push_back( tmp );
				} // if
				
				if ( stricmp( instr.c_str(), "start" ) == 0 ) ;
				else if ( stricmp( instr.c_str(), "end" ) == 0 ) {
					for ( int j = 0 ; j < ltr_tab_.size() ; j++ ) {
						ltr_tab_[j].addr = location_counter_;
						ltr_table_.push_back( ltr_tab_[j] );
						
						if ( ltr_tab_[j].type == 6 ) location_counter_ += 1; // X'F1'
						else location_counter_ += 3; // C'EOF'
					} // end for
					
					ltr_tab_.clear();
				} // else if
				else if ( stricmp( instr.c_str(), "base" ) == 0 ) ;
				else if ( stricmp( instr.c_str(), "ltorg" ) == 0 ) {
					for ( int j = 0 ; j < ltr_tab_.size() ; j++ ) {
						ltr_tab_[j].addr = location_counter_;
						ltr_table_.push_back( ltr_tab_[j] );
						
						if ( ltr_tab_[j].type == 6 ) location_counter_ += 1; // X'F1'
						else location_counter_ += 3; // C'EOF'
					} // end for
					
					ltr_tab_.clear();
				} // else if
				else if ( stricmp( instr.c_str(), "byte" ) == 0 ) {
					index = fetchOp( i, 2 ); // fetch first op
					if ( output_token_[i][index].token_type == 6 ) // X'F1'
						location_counter_ += 1;
					else location_counter_ += 3; // C'EOF'
				} // else if
				else if ( stricmp( instr.c_str(), "word" ) == 0 ) {
					location_counter_ += 3;
				} // else if
				else if ( stricmp( instr.c_str(), "resb" ) == 0 ) {
					index = fetchOp( i, 2 ); // fetch first op
					op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
					location_counter_ += strtol(op.c_str(), NULL, 10);
				} // else if
				else if ( stricmp( instr.c_str(), "resw" ) == 0 ) {
					index = fetchOp( i, 2 ); // fetch first op
					op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
					location_counter_ += ( strtol(op.c_str(), NULL, 10) * 3 );
				} // else if
				else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
						location_counter_ = tmp_loc;
				} // else if
				else if ( whichFormat( instr ) ) location_counter_ += whichFormat( instr );
				else cout << "instruction not found in line " << 5*(i+1) << " !\n";
				
			} // if
			else if ( result == 2 ) {
				instr = token_table_[output_token_[i][0].token_type-1][output_token_[i][0].token_value];
				if ( stricmp( instr.c_str(), "start" ) == 0 ) {
					bool minus = false;
					int addr1 = 0, addr2 = 0, sym_index = 0;
					index = fetchOp( i, 1 ); // fetch op1(suppose that there is always a op1)
					if ( isSymOrInt( i, index ) == 1 ) { // symbol
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						sym_index = inSymboyTable( op );
						addr1 = symbol_table_[sym_index].addr;
					} // if
					else if ( isSymOrInt( i, index ) == 2 ) { // int
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						addr1 = strtol(op.c_str(), NULL, 16);
					} // else if
					else if ( index == 0 ) addr1 = location_counter_;
					else addr1 = 0;
					
					index = fetchOp( i, index + 1 ); // fetch op2
					if ( index != 0 ) { // there is op2
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr2 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr2 = strtol(op.c_str(), NULL, 16);
						} // else if
						else addr2 = 0;
						
						if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
							minus = true;
					} // if
					
					if ( minus ) location_counter_ = addr1 - addr2;
					else location_counter_ = addr1 + addr2;
				} // if
				else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
					tmp_loc = location_counter_;
					bool minus = false;
					int addr1 = 0, addr2 = 0, sym_index = 0;
					index = fetchOp( i, 1 ); // fetch op1(suppose that there is always a op1)
					if ( isSymOrInt( i, index ) == 1 ) { // symbol
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						sym_index = inSymboyTable( op );
						addr1 = symbol_table_[sym_index].addr;
					} // if
					else if ( isSymOrInt( i, index ) == 2 ) { // int
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						addr1 = strtol(op.c_str(), NULL, 10);
					} // else if
					else if ( index == 0 ) addr1 = location_counter_;
					else addr1 = 0;
					
					index = fetchOp( i, index + 1 ); // fetch op2
					if ( index != 0 ) { // there is op2
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr2 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr2 = strtol(op.c_str(), NULL, 10);
						} // else if
						else addr2 = 0;
						
						if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
							minus = true;
					} // if
					
					if ( minus ) location_counter_ = addr1 - addr2;
					else location_counter_ = addr1 + addr2;
				} // else if
					 
				ltr = hasLTR( i, 1 );
				if ( ! ltr.empty() && inLtrTab( ltr ) == -1 ) { // hasLTR && not in ltr table
					string code;
					index = fetchOp( i, 1 ); // fetch first op
					if ( output_token_[i][index].token_type == 6 ) { // X'F1'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						code += op;
					} // if
					else { // C'EOF'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) 
							code.append("00");
							
						char buf[20];
						for ( int i = 0 ; i < op.size() ; i++ ) {
							sprintf( buf, "%X", int(op[i]));
							code += buf;
						} // end for
					} // else
						
					tmp.type = output_token_[i][index].token_type;
					tmp.code = code;
					tmp.symbol = ltr;
					ltr_tab_.push_back( tmp );
				} // if	
				
				if ( stricmp( instr.c_str(), "start" ) == 0 ) ;
				else if ( stricmp( instr.c_str(), "end" ) == 0 ) {
					for ( int j = 0 ; j < ltr_tab_.size() ; j++ ) {
						ltr_tab_[j].addr = location_counter_;
						ltr_table_.push_back( ltr_tab_[j] );
						
						if ( ltr_tab_[j].type == 6 ) location_counter_ += 1; // X'F1'
						else location_counter_ += 3; // C'EOF'
					} // end for
					
					ltr_tab_.clear();
				} // else if
				else if ( stricmp( instr.c_str(), "base" ) == 0 ) ;
				else if ( stricmp( instr.c_str(), "ltorg" ) == 0 ) {
					for ( int j = 0 ; j < ltr_tab_.size() ; j++ ) {
						ltr_tab_[j].addr = location_counter_;
						ltr_table_.push_back( ltr_tab_[j] );
						
						if ( ltr_tab_[j].type == 6 ) location_counter_ += 1; // X'F1'
						else location_counter_ += 3; // C'EOF'
					} // end for
					
					ltr_tab_.clear();
				} // else if
				else if ( stricmp( instr.c_str(), "byte" ) == 0 ) {
					index = fetchOp( i, 1 ); // fetch first op
					if ( output_token_[i][index].token_type == 6 ) // X'F1'
						location_counter_ += 1;
					else location_counter_ += 3; // C'EOF'
				} // else if
				else if ( stricmp( instr.c_str(), "word" ) == 0 ) {
					location_counter_ += 3;
				} // else if
				else if ( stricmp( instr.c_str(), "resb" ) == 0 ) {
					index = fetchOp( i, 1 ); // fetch first op
					op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
					location_counter_ += strtol(op.c_str(), NULL, 10);
				} // else if
				else if ( stricmp( instr.c_str(), "resw" ) == 0 ) {
					index = fetchOp( i, 1 ); // fetch first op
					op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
					location_counter_ += ( strtol(op.c_str(), NULL, 10) * 3 );
				} // else if
				else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
					location_counter_ = tmp_loc;
				} // else if
				else if ( whichFormat( instr ) ) location_counter_ += whichFormat( instr );
				else cout << "instruction not found in line " << 5*(i+1) << " !\n";
				
			} // else if
			else if ( result == 3 ) { // +instr
				ltr = hasLTR( i, 2 );
				if ( ! ltr.empty() && inLtrTab( ltr ) == -1 ) { // hasLTR && not in ltr table
					string code;
					index = fetchOp( i, 2 ); // fetch first op
					if ( output_token_[i][index].token_type == 6 ) { // X'F1'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						code += op;
					} // if
					else { // C'EOF'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) 
							code.append("00");
							
						char buf[20];
						for ( int i = 0 ; i < op.size() ; i++ ) {
							sprintf( buf, "%X", int(op[i]));
							code += buf;
						} // end for
					} // else
						
					tmp.type = output_token_[i][index].token_type;
					tmp.code = code;
					tmp.symbol = ltr;
					ltr_tab_.push_back( tmp );
				} // if
			
				location_counter_ += 4;
			} // else if
			else if ( result == 4 ) { // symbol +instr
				symbol = token_table_[output_token_[i][0].token_type-1][output_token_[i][0].token_value];
				
				tmp.addr = location_counter_;
				tmp.symbol = symbol;
				if ( inSymboyTable( tmp.symbol ) != -1 ) cout << "duplicated definition in line " << 5*(i+1) << " !\n";
				else symbol_table_.push_back( tmp );
				
				ltr = hasLTR( i, 3 );
				if ( ! ltr.empty() && inLtrTab( ltr ) == -1 ) { // hasLTR && not in ltr table
					string code;
					index = fetchOp( i, 3 ); // fetch first op
					if ( output_token_[i][index].token_type == 6 ) { // X'F1'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						code += op;
					} // if
					else { // C'EOF'
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) 
							code.append("00");
							
						char buf[20];
						for ( int i = 0 ; i < op.size() ; i++ ) {
							sprintf( buf, "%X", int(op[i]));
							code += buf;
						} // end for
					} // else
						
					tmp.type = output_token_[i][index].token_type;
					tmp.code = code;
					tmp.symbol = ltr;
					ltr_tab_.push_back( tmp );
				} // if
				
				location_counter_ += 4;
			} // else if
			else if ( result == 5 ) ; // comments or blank line
			else cout << "syntax error in line " << 5*(i+1) << " !\n";
		} // end for
		
	} // end pass1()
	
	void pass2( string file_name ) { // undefined
		fstream outFile;
        file_name = "output_of_" + file_name + ".txt";
        outFile.open( file_name.c_str(), fstream::out );
        if ( outFile.is_open() ) { // open successfully
			location_counter_ = 0;
			int index = 0, result = 0, tmp_loc = 0;
			string op1, op2;
			string symbol, instr, op, objcode, ltr;
			SymbolType tmp;
			outFile << "Line\t" << "Loc\t\t" << left << setw(32) << "Source code" << left << "Object code\n" << right;
			outFile << "----\t" << "---\t\t" << left << setw(32) << "-----------" << left << "-----------\n" << right;
			for ( int i = 0 ; i < output_token_.size() ; i++ ) {
				result = syntaxAnalysis( i );
				objcode.clear();
				if ( result == 1 ) {
					symbol = token_table_[output_token_[i][0].token_type-1][output_token_[i][0].token_value];
					instr = token_table_[output_token_[i][1].token_type-1][output_token_[i][1].token_value];
					if ( stricmp( instr.c_str(), "start" ) == 0 ) {
						bool minus = false;
						int addr1 = 0, addr2 = 0, sym_index = 0;
						index = fetchOp( i, 2 ); // fetch op1(suppose that there is always a op1)
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr1 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr1 = strtol(op.c_str(), NULL, 16);
						} // else if
						else if ( index == 0 ) addr1 = location_counter_;
						else addr1 = 0;
						
						index = fetchOp( i, index + 1 ); // fetch op2
						if ( index != 0 ) { // there is op2
							if ( isSymOrInt( i, index ) == 1 ) { // symbol
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								sym_index = inSymboyTable( op );
								addr2 = symbol_table_[sym_index].addr;
							} // if
							else if ( isSymOrInt( i, index ) == 2 ) { // int
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								addr2 = strtol(op.c_str(), NULL, 16);
							} // else if
							else addr2 = 0;
							
							if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
								minus = true;
						} // if
						
						if ( minus ) location_counter_ = addr1 - addr2;
						else location_counter_ = addr1 + addr2;
					} // if
					else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
						tmp_loc = location_counter_;
						bool minus = false;
						int addr1 = 0, addr2 = 0, sym_index = 0;
						index = fetchOp( i, 2 ); // fetch op1(suppose that there is always a op1)
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr1 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr1 = strtol(op.c_str(), NULL, 10);
						} // else if
						else if ( index == 0 ) addr1 = location_counter_;
						else addr1 = 0;
						
						index = fetchOp( i, index + 1 ); // fetch op2
						if ( index != 0 ) { // there is op2
							if ( isSymOrInt( i, index ) == 1 ) { // symbol
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								sym_index = inSymboyTable( op );
								addr2 = symbol_table_[sym_index].addr;
							} // if
							else if ( isSymOrInt( i, index ) == 2 ) { // int
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								addr2 = strtol(op.c_str(), NULL, 10);
							} // else if
							else addr2 = 0;
							
							if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
								minus = true;
						} // if
						
						if ( minus ) location_counter_ = addr1 - addr2;
						else location_counter_ = addr1 + addr2;
					} // else if
					
					outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << '\t'; 
					if ( stricmp( instr.c_str(), "end" ) != 0 && stricmp( instr.c_str(), "base" ) != 0 && stricmp( instr.c_str(), "ltorg" ) != 0 ) // != end || != base
						outFile << hex << uppercase  << setw(4) << setfill('0') << location_counter_ << '\t';
					
					ltr = hasLTR( i, 2 );
					if ( ! ltr.empty() && inLtrTab( ltr ) == -1 ) { // hasLTR && not in ltr table
						string code;
						index = fetchOp( i, 2 ); // fetch first op
						if ( output_token_[i][index].token_type == 6 ) { // X'F1'
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							code += op;
						} // if
						else { // C'EOF'
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) 
								code.append("00");
								
							char buf[20];
							for ( int i = 0 ; i < op.size() ; i++ ) {
								sprintf( buf, "%X", int(op[i]));
								code += buf;
							} // end for
						} // else
							
						tmp.type = output_token_[i][index].token_type;
						tmp.code = code;
						tmp.symbol = ltr;
						ltr_tab_.push_back( tmp );
					} // if
					
					if ( stricmp( instr.c_str(), "start" ) == 0 ) ;
					else if ( stricmp( instr.c_str(), "end" ) == 0 || 
							  stricmp( instr.c_str(), "ltorg" ) == 0 ) {
						outFile << left << setw(24) << setfill(' ') << input_prog_[i] << "\t\t"
								<< left <<  setw(8) << setfill(' ') << objcode << '\n' << right;
						for ( int j = 0 ; j < ltr_tab_.size() ; j++ ) {
							outFile << dec << setw(4) << setfill(' ') << " " << '\t'; 
							outFile << hex << uppercase  <<  setw(4) << setfill('0') << location_counter_ << '\t';
							outFile << left << setw(8) << setfill(' ') << "*"
											<< setw(8) << setfill(' ') << ltr_tab_[j].symbol
											<< setw(8) << setfill(' ') << " " << "\t\t"
									<< left <<  setw(8) << setfill(' ') << ltr_tab_[j].code << '\n' << right;
							
							if ( ltr_tab_[j].type == 6 ) location_counter_ += 1; // X'F1'
							else location_counter_ += 3; // C'EOF'
						} // end for
						
						ltr_tab_.clear();
					} // else if
					else if ( stricmp( instr.c_str(), "base" ) == 0 ) {
						index = fetchOp( i, 2 ); // fetch first op
						string symbol = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						index = inSymboyTable( symbol );
						if ( index != -1 ) // defined symbol
							base_reg_ = symbol_table_[index].addr;
						else cout << "undefined symbol in line " << 5*(i+1) << " !\n";
					} // else if
					else if ( stricmp( instr.c_str(), "byte" ) == 0 ) {
						index = fetchOp( i, 2 ); // fetch first op
						if ( output_token_[i][index].token_type == 6 ) { // X'F1'
							location_counter_ += 1;
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							objcode += op;
						} // if
						else { // C'EOF'
							location_counter_ += 3; 
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) {
								objcode.append("00");
							} // end for
							
							char buf[20];
							for ( int i = 0 ; i < op.size() ; i++ ) {
								sprintf( buf, "%X", int(op[i]));
								objcode += buf;
							} // end for
						} // else
					} // else if
					else if ( stricmp( instr.c_str(), "word" ) == 0 ) {
						location_counter_ += 3;
						index = fetchOp( i, 2 );
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						for ( int i = 0 ; i < ( 6 - op.size() ) ; i++ ) {
							objcode.append("0");
						} // end for
						
						char buf[20];
						sprintf( buf, "%X", strtol(op.c_str(), NULL, 10) );
						objcode += buf;
					} // else if
					else if ( stricmp( instr.c_str(), "resb" ) == 0 ) {
						index = fetchOp( i, 2 ); // fetch first op
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						location_counter_ += strtol(op.c_str(), NULL, 10);
					} // else if
					else if ( stricmp( instr.c_str(), "resw" ) == 0 ) {
						index = fetchOp( i, 2 ); // fetch first op
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						location_counter_ += ( strtol(op.c_str(), NULL, 10) * 3 );
					} // else if
					else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
						location_counter_ = tmp_loc;
					} // else if
					else if ( whichFormat( instr ) ) {
						location_counter_ += whichFormat( instr );
						if ( whichFormat( instr ) == 1 ) {
							objcode = getOpcode( instr );
						} // if
						else if ( whichFormat( instr ) == 2 ) {
							objcode = getOpcode( instr );
							index = fetchOp( i, 2 );
							if ( index != 0 ) { // r1
								char buf[20];
								sprintf( buf, "%X", output_token_[i][index].token_value);
								objcode += buf;
							} // if
							else objcode += "0";
							
							index = fetchOp( i, 3 ); // r2
							if ( index != 0 ) {
								char buf[20];
								sprintf( buf, "%X", output_token_[i][index].token_value);
								objcode += buf;
							} // if op1
							else objcode += "0";
							
						} // else if
						else if ( whichFormat( instr ) == 3 ) {
							int tmp_index = 0;
							string flag;
							tmp_index = fetchOp( i, 2 );
							// opcode
							objcode = opToBin( getOpcode( instr ) );
							
							// n, i
							if ( g_SIC ) flag.append( "00" );
							else if ( tmp_index != 0 ) {
								string deli = token_table_[output_token_[i][1].token_type-1][output_token_[i][1].token_value];
								
								if ( deli == "@" ) flag.append( "10" );
								else if ( deli == "#" ) flag.append( "01" );
								// else if ( SIC ) flag.append( "00" );
								else flag.append( "11" ); // SIC/XE
							} // if 
							else flag.append( "11" );
							// x
							index = fetchOp( i, 2 ); // op1
							index = fetchOp( i, index + 1 ); // op2
							if ( output_token_[i][index].token_type == 3 && output_token_[i][index].token_value == 1 )
								flag.append( "1" );
							else flag.append( "0" );
							
							if ( g_SIC ) { // SIC
								// addr
								int addr = 0; // 15-bit
								// addr
								tmp_index = fetchOp( i, 2 ); // op1
								if ( tmp_index == 0 ) { // no operand
									addr = 0;
								} // if
								else {
									string symbol = token_table_[output_token_[i][tmp_index].token_type-1][output_token_[i][tmp_index].token_value];
									index = inSymboyTable( symbol );
									if ( index != -1 ) { // defined symbol
										addr = symbol_table_[index].addr;
									} // if
									else if ( isOperand( i, tmp_index ) ) { // integer
										addr = strtol( symbol.c_str(),NULL, 10 );						
									} // else if
									else { // undefined symbol
										cout << "undefined symbol in line " << 5*(i+1) << " !\n";
									} // else
								} // else

								char buf[33];
								itoa( addr, buf, 2 );
								string buf_addr = buf;
								string real_addr;
								
								if ( buf_addr.size() > 15 ) {
									real_addr = buf_addr.substr( buf_addr.size() - 15, 15 );
								} // if
								else {
									for ( int i = 0 ; i < ( 15 - buf_addr.size() ) ; i++ )
										real_addr.append("0");
									real_addr += buf_addr;
								} // else
								
								string bin = objcode + flag + real_addr;
								objcode = binToHex( bin );
							} // if
							else { // SIC/XE
								//disp
								int disp = 0;
								tmp_index = fetchOp( i, 2 ); // op1
								if ( tmp_index == 0 ) { // no operand
									flag.append( "000" ); // b, p, e
									disp = 0;
								} // if
								else {
									string symbol = token_table_[output_token_[i][tmp_index].token_type-1][output_token_[i][tmp_index].token_value];
									index = inSymboyTable( symbol );
									string ltr = hasLTR( i, 2 );
									int ltr_index = inLtrTable( ltr );
									if ( index != -1 ) { // defined symbol
										disp = symbol_table_[index].addr - location_counter_;
										if ( over3Bytes( disp ) ){
											disp = symbol_table_[index].addr - base_reg_; // base
											flag.append( "100" ); // b, p, e
										} // if
										else flag.append( "010" ); // b, p, e
									} // if
									else if ( ltr_index != -1 ) {
										disp = ltr_table_[ltr_index].addr - location_counter_;
										if ( over3Bytes( disp ) ){
											disp = symbol_table_[index].addr - base_reg_; // base
											flag.append( "100" ); // b, p, e
										} // if
										else flag.append( "010" ); // b, p, e
									} // else if
									else if ( isOperand( i, tmp_index ) ) { // integer
										flag.append( "000" ); // b, p, e
										disp = strtol( symbol.c_str(),NULL, 10 );						
									} // else if
									else { // undefined symbol
										flag.append( "000" ); // b, p, e
										cout << "undefined symbol in line " << 5*(i+1) << " !\n";
									} // else
								} // else
								
								char buf[33];
								itoa( disp, buf, 2 );
								string buf_disp = buf;
								string real_disp;
								if ( buf_disp.size() > 12 ) {
									real_disp = buf_disp.substr( buf_disp.size() - 12, 12 );
								} // if
								else {
									for ( int i = 0 ; i < ( 12 - buf_disp.size() ) ; i++ )
										real_disp.append("0");
									real_disp += buf_disp;
								} // else
								
								string bin = objcode + flag + real_disp;
								objcode = binToHex( bin );
							} // else
						} // else if
					} // else if
					else {
						outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << '\t' << "instruction not found!\n";
						continue;
					} // else
					
					if ( stricmp( instr.c_str(), "end" ) != 0 && stricmp( instr.c_str(), "ltorg" ) != 0 )
						outFile << left << setw(24) << setfill(' ') << input_prog_[i] << "\t\t"
								<< left <<  setw(8) << setfill(' ') << objcode << '\n' << right;
				} // if 1
				else if ( result == 2 ) {
					instr = token_table_[output_token_[i][0].token_type-1][output_token_[i][0].token_value];
					if ( stricmp( instr.c_str(), "start" ) == 0 ) {
						bool minus = false;
						int addr1 = 0, addr2 = 0, sym_index = 0;
						index = fetchOp( i, 1 ); // fetch op1(suppose that there is always a op1)
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr1 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr1 = strtol(op.c_str(), NULL, 16);
						} // else if
						else if ( index == 0 ) addr1 = location_counter_;
						else addr1 = 0;
						
						index = fetchOp( i, index + 1 ); // fetch op2
						if ( index != 0 ) { // there is op2
							if ( isSymOrInt( i, index ) == 1 ) { // symbol
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								sym_index = inSymboyTable( op );
								addr2 = symbol_table_[sym_index].addr;
							} // if
							else if ( isSymOrInt( i, index ) == 2 ) { // int
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								addr2 = strtol(op.c_str(), NULL, 16);
							} // else if
							else addr2 = 0;
							
							if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
								minus = true;
						} // if
						
						if ( minus ) location_counter_ = addr1 - addr2;
						else location_counter_ = addr1 + addr2;
					} // if
					else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
						tmp_loc = location_counter_;
						bool minus = false;
						int addr1 = 0, addr2 = 0, sym_index = 0;
						index = fetchOp( i, 1 ); // fetch op1(suppose that there is always a op1)
						if ( isSymOrInt( i, index ) == 1 ) { // symbol
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							sym_index = inSymboyTable( op );
							addr1 = symbol_table_[sym_index].addr;
						} // if
						else if ( isSymOrInt( i, index ) == 2 ) { // int
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							addr1 = strtol(op.c_str(), NULL, 10);
						} // else if
						else if ( index == 0 ) addr1 = location_counter_;
						else addr1 = 0;
						
						index = fetchOp( i, index + 1 ); // fetch op2
						if ( index != 0 ) { // there is op2
							if ( isSymOrInt( i, index ) == 1 ) { // symbol
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								sym_index = inSymboyTable( op );
								addr2 = symbol_table_[sym_index].addr;
							} // if
							else if ( isSymOrInt( i, index ) == 2 ) { // int
								op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
								addr2 = strtol(op.c_str(), NULL, 10);
							} // else if
							else addr2 = 0;
							
							if ( token_table_[output_token_[i][index-1].token_type-1][output_token_[i][index-1].token_value] == "-" )
								minus = true;
						} // if
						
						if ( minus ) location_counter_ = addr1 - addr2;
						else location_counter_ = addr1 + addr2;
					} // else if
					
					outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << '\t'; 
					if ( stricmp( instr.c_str(), "end" ) != 0 && stricmp( instr.c_str(), "base" ) != 0 && stricmp( instr.c_str(), "ltorg" ) != 0 ) // != end || != base
						outFile << hex << uppercase  <<  setw(4) << setfill('0') << location_counter_ << '\t';
						 
					ltr = hasLTR( i, 1 );
					if ( ! ltr.empty() && inLtrTab( ltr ) == -1 ) { // hasLTR && not in ltr table
						string code;
						index = fetchOp( i, 1 ); // fetch first op
						if ( output_token_[i][index].token_type == 6 ) { // X'F1'
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							code += op;
						} // if
						else { // C'EOF'
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) 
								code.append("00");
								
							char buf[20];
							for ( int i = 0 ; i < op.size() ; i++ ) {
								sprintf( buf, "%X", int(op[i]));
								code += buf;
							} // end for
						} // else
							
						tmp.type = output_token_[i][index].token_type;
						tmp.code = code;
						tmp.symbol = ltr;
						ltr_tab_.push_back( tmp );
					} // if
					
					if ( stricmp( instr.c_str(), "start" ) == 0 ) ;
					else if ( stricmp( instr.c_str(), "end" ) == 0 || 
							  stricmp( instr.c_str(), "ltorg" ) == 0 ) {
						outFile << left << setw(24) << setfill(' ') << input_prog_[i] << "\t\t"
								<< left <<  setw(8) << setfill(' ') << objcode << '\n' << right;
						for ( int j = 0 ; j < ltr_tab_.size() ; j++ ) {
							outFile << dec << setw(4) << setfill(' ') << " " << '\t'; 
							outFile << hex << uppercase  <<  setw(4) << setfill('0') << location_counter_ << '\t';
							outFile << left << setw(8) << setfill(' ') << "*"
											<< setw(8) << setfill(' ') << ltr_tab_[j].symbol
											<< setw(8) << setfill(' ') << " " << "\t\t"
									<< left <<  setw(8) << setfill(' ') << ltr_tab_[j].code << '\n' << right;
							
							if ( ltr_tab_[j].type == 6 ) location_counter_ += 1; // X'F1'
							else location_counter_ += 3; // C'EOF'
						} // end for
						
						ltr_tab_.clear();
					} // else if
					else if ( stricmp( instr.c_str(), "base" ) == 0 ) {
						index = fetchOp( i, 1 ); // fetch first op
						string symbol = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						index = inSymboyTable( symbol );
						if ( index != -1 ) // defined symbol
							base_reg_ = symbol_table_[index].addr;
						else cout << "undefined symbol in line " << 5*(i+1) << " !\n";
					} // else if
					else if ( stricmp( instr.c_str(), "byte" ) == 0 ) {
						index = fetchOp( i, 1 ); // fetch first op
						if ( output_token_[i][index].token_type == 6 ) { // X'F1'
							location_counter_ += 1;
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							objcode += op;
						} // if
						else { // C'EOF'
							location_counter_ += 3; 
							op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
							for ( int i = 0 ; i < ( 3 - op.size() ) ; i++ ) {
								objcode.append("00");
							} // end for
							
							char buf[20];
							for ( int i = 0 ; i < op.size() ; i++ ) {
								sprintf( buf, "%X", int(op[i]));
								objcode += buf;
							} // end for
						} // else
					} // else if
					else if ( stricmp( instr.c_str(), "word" ) == 0 ) {
						location_counter_ += 3;
						index = fetchOp( i, 1 );
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						for ( int i = 0 ; i < ( 6 - op.size() ) ; i++ ) {
							objcode.append("0");
						} // end for
						
						char buf[20];
						sprintf( buf, "%X", strtol(op.c_str(), NULL, 10) );
						objcode += buf;
					} // else if
					else if ( stricmp( instr.c_str(), "resb" ) == 0 ) {
						index = fetchOp( i, 1 ); // fetch first op
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						location_counter_ += strtol(op.c_str(), NULL, 10);
					} // else if
					else if ( stricmp( instr.c_str(), "resw" ) == 0 ) {
						index = fetchOp( i, 1 ); // fetch first op
						op = token_table_[output_token_[i][index].token_type-1][output_token_[i][index].token_value];
						location_counter_ += ( strtol(op.c_str(), NULL, 10) * 3 );
					} // else if
					else if ( stricmp( instr.c_str(), "equ" ) == 0 ) {
						location_counter_ = tmp_loc;
					} // else if
					else if ( whichFormat( instr ) ) {
						location_counter_ += whichFormat( instr );
						if ( whichFormat( instr ) == 1 ) {
							objcode = getOpcode( instr );
						} // if
						else if ( whichFormat( instr ) == 2 ) {
							objcode = getOpcode( instr );
							index = fetchOp( i, 1 );
							if ( index != 0 ) { // r1
								char buf[20];
								sprintf( buf, "%X", output_token_[i][index].token_value);
								objcode += buf;
							} // if
							else objcode += "0";
							
							index = fetchOp( i, 2 ); // r2
							if ( index != 0 ) {
								char buf[20];
								sprintf( buf, "%X", output_token_[i][index].token_value);
								objcode += buf;
							} // if op1
							else objcode += "0";
						} // else if
						else if ( whichFormat( instr ) == 3 ) {
							
							int tmp_index = 0;
							string flag;
							tmp_index = fetchOp( i, 1 );
							// opcode
							objcode = opToBin( getOpcode( instr ) );
							
							// n, i
							if ( g_SIC ) flag.append( "00" );
							else if ( tmp_index != 0 ) {
								string deli = token_table_[output_token_[i][1].token_type-1][output_token_[i][1].token_value];
								
								if ( deli == "@" ) flag.append( "10" );
								else if ( deli == "#" ) flag.append( "01" );
								else flag.append( "11" ); // SIC/XE
							} // if 
							else flag.append( "11" );
							
							// x
							index = fetchOp( i, 1 ); // op1
							index = fetchOp( i, index + 1 ); // op2
							if ( output_token_[i][index].token_type == 3 && output_token_[i][index].token_value == 1 )
								flag.append( "1" );
							else flag.append( "0" );
							
							if ( g_SIC ) { // SIC
								// addr
								int addr = 0; // 15-bit
								// addr
								tmp_index = fetchOp( i, 1 ); // op1
								if ( tmp_index == 0 ) { // no operand
									addr = 0;
								} // if
								else {
									string symbol = token_table_[output_token_[i][tmp_index].token_type-1][output_token_[i][tmp_index].token_value];
									index = inSymboyTable( symbol );
									if ( index != -1 ) { // defined symbol
										addr = symbol_table_[index].addr;
									} // if
									else if ( isOperand( i, tmp_index ) ) { // integer
										addr = strtol( symbol.c_str(),NULL, 10 );						
									} // else if
									else { // undefined symbol
										cout << "undefined symbol in line " << 5*(i+1) << " !\n";
									} // else
								} // else

								char buf[33];
								itoa( addr, buf, 2 );
								string buf_addr = buf;
								string real_addr;
								
								if ( buf_addr.size() > 15 ) {
									real_addr = buf_addr.substr( buf_addr.size() - 15, 15 );
								} // if
								else {
									for ( int i = 0 ; i < ( 15 - buf_addr.size() ) ; i++ )
										real_addr.append("0");
									real_addr += buf_addr;
								} // else
								
								string bin = objcode + flag + real_addr;
								objcode = binToHex( bin );
							} // if
							else { // SIC/XE
								//disp
								int disp = 0;
								tmp_index = fetchOp( i, 1 ); // op1
								if ( tmp_index == 0 ) { // no operand
									flag.append( "000" ); // b, p, e
									disp = 0;
								} // if
								else {
									string symbol = token_table_[output_token_[i][tmp_index].token_type-1][output_token_[i][tmp_index].token_value];
									index = inSymboyTable( symbol );
									string ltr = hasLTR( i, 1 );
									int ltr_index = inLtrTable( ltr );
									if ( index != -1 ) { // defined symbol
										disp = symbol_table_[index].addr - location_counter_;
										if ( over3Bytes( disp ) ) {
											disp = symbol_table_[index].addr - base_reg_;
											flag.append( "100" ); // b, p, e
										} // if
										else flag.append( "010" ); // b, p, e
									} // if
									else if ( ltr_index != -1 ) {
										disp = ltr_table_[ltr_index].addr - location_counter_;
										if ( over3Bytes( disp ) ){
											disp = symbol_table_[index].addr - base_reg_; // base
											flag.append( "100" ); // b, p, e
										} // if
										else flag.append( "010" ); // b, p, e
									} // else if
									else if ( isOperand( i, tmp_index ) ) { // integer
										flag.append( "000" ); // b, p, e
										disp = strtol( symbol.c_str(),NULL, 10 );						
									} // else if
									else { // undefined symbol
										flag.append( "000" ); // b, p, e
										cout << "undefined symbol in line " << 5*(i+1) << " !\n";
									} // else
								} // else
								
								char buf[33];
								itoa( disp, buf, 2 );
								
								string buf_disp = buf;
								string real_disp;
								if ( buf_disp.size() > 12 ) {
									real_disp = buf_disp.substr( buf_disp.size() - 12, 12 );
								} // if
								else {
									for ( int i = 0 ; i < ( 12 - buf_disp.size() ) ; i++ )
										real_disp.append("0");
									real_disp += buf_disp;
								} // else
								
								string bin = objcode + flag + real_disp;
								objcode = binToHex( bin );
							} // else
						} // else if
					} // else if
					else {
						outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << '\t' << "instruction not found!\n";
						continue;
					} // else
					
				
					if ( stricmp( instr.c_str(), "end" ) != 0 && stricmp( instr.c_str(), "ltorg" ) != 0 )
						outFile << left << setw(24) << setfill(' ') << input_prog_[i] << "\t\t"
								<< left <<  setw(8) << setfill(' ') << objcode << '\n' << right;
				} // else if 2
				else if ( result == 3 ) { // +instr
					outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << '\t'; 
					outFile << hex << uppercase  << setw(4) << setfill('0') << location_counter_ << '\t';
					
					instr = token_table_[output_token_[i][1].token_type-1][output_token_[i][1].token_value];
					location_counter_ += 4;
					
					int addr = 0;
					int tmp_index = 0;
					string flag;
					tmp_index = fetchOp( i, 2 );
					// opcode
					objcode = opToBin( getOpcode( instr ) );
					// n, i
					if ( tmp_index != 0 ) {
						string deli = token_table_[output_token_[i][2].token_type-1][output_token_[i][2].token_value];
						
						if ( deli == "@" ) flag.append( "10" );
						else if ( deli == "#" ) flag.append( "01" );
						else flag.append( "11" ); // SIC/XE
					} // if 
					else flag.append( "11" );
					// x
					index = fetchOp( i, 1 ); // op1
					index = fetchOp( i, index + 1 ); // op2
					if ( output_token_[i][index].token_type == 3 && output_token_[i][index].token_value == 1 )
						flag.append( "1" );
					else flag.append( "0" );
					// b, p, e
					flag.append( "001" );
					// addr
					tmp_index = fetchOp( i, 1 ); // op1
					if ( tmp_index == 0 ) { // no operand
						addr = 0;
					} // if
					else {
						string symbol = token_table_[output_token_[i][tmp_index].token_type-1][output_token_[i][tmp_index].token_value];
						index = inSymboyTable( symbol );
						if ( index != -1 ) { // defined symbol
							addr = symbol_table_[index].addr;
						} // if
						else if ( isOperand( i, tmp_index ) ) { // integer
							addr = strtol( symbol.c_str(),NULL, 10 );						
						} // else if
						else { // undefined symbol
							cout << "undefined symbol in line " << 5*(i+1) << " !\n";
						} // else
					} // else

					char buf[33];
					itoa( addr, buf, 2 );
					string buf_addr = buf;
					string real_addr;
					
					if ( buf_addr.size() > 20 ) {
						real_addr = buf_addr.substr( buf_addr.size() - 20, 20 );
					} // if
					else {
						for ( int i = 0 ; i < ( 20 - buf_addr.size() ) ; i++ )
							real_addr.append("0");
						real_addr += buf_addr;
					} // else
					
					string bin = objcode + flag + real_addr;
					objcode = binToHex( bin );
					
					outFile << left << setw(24) << setfill(' ') << input_prog_[i] << "\t\t"
							<< left <<  setw(8) << setfill(' ') << objcode << '\n' << right;
				} // else if 3
				else if ( result == 4 ) { // symbol +instr
					outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << '\t'; 
					outFile << hex << uppercase  << setw(4) << setfill('0') << location_counter_ << '\t';
					
					instr = token_table_[output_token_[i][2].token_type-1][output_token_[i][2].token_value];
					location_counter_ += 4;
					int addr = 0;
					int tmp_index = 0;
					string flag;
					tmp_index = fetchOp( i, 3 );
					// opcode
					objcode = opToBin( getOpcode( instr ) );
					// n, i
					if ( tmp_index != 0 ) {
						string deli = token_table_[output_token_[i][3].token_type-1][output_token_[i][3].token_value];
						
						if ( deli == "@" ) flag.append( "10" );
						else if ( deli == "#" ) flag.append( "01" );
						else flag.append( "11" ); // SIC/XE
					} // if 
					else flag.append( "11" );
					// x
					index = fetchOp( i, 2 ); // op1
					index = fetchOp( i, index + 1 ); // op2
					if ( output_token_[i][index].token_type == 3 && output_token_[i][index].token_value == 1 )
						flag.append( "1" );
					else flag.append( "0" );
					// b, p, e
					flag.append( "001" );
					// addr
					tmp_index = fetchOp( i, 2 ); // op1
					if ( tmp_index == 0 ) { // no operand
						addr = 0;
					} // if
					else {
						string symbol = token_table_[output_token_[i][tmp_index].token_type-1][output_token_[i][tmp_index].token_value];
						index = inSymboyTable( symbol );
						if ( index != -1 ) { // defined symbol
							addr = symbol_table_[index].addr;
						} // if
						else if ( isOperand( i, tmp_index ) ) { // integer
							addr = strtol( symbol.c_str(),NULL, 10 );						
						} // else if
						else { // undefined symbol
							cout << "undefined symbol in line " << 5*(i+1) << " !\n";
						} // else
					} // else

					char buf[33];
					itoa( addr, buf, 2 );
					string buf_addr = buf;
					string real_addr;
					
					if ( buf_addr.size() > 20 ) {
						real_addr = buf_addr.substr( buf_addr.size() - 20, 20 );
					} // if
					else {
						for ( int i = 0 ; i < ( 20 - buf_addr.size() ) ; i++ )
							real_addr.append("0");
						real_addr += buf_addr;
					} // else
					
					string bin = objcode + flag + real_addr;
					objcode = binToHex( bin );
					
					outFile << left << setw(24) << setfill(' ') << input_prog_[i] << "\t\t"
							<< left <<  setw(8) << setfill(' ') << objcode << '\n' << right;
				} // else if 4
				else if ( result == 5 ) { // comments or blank line
					outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << "\t\t\t" << input_prog_[i] << endl ;
				} // else if
				else outFile << dec << setw(4) << setfill(' ') << 5 * (i+1) << '\t' << "syntax error\n";
			} // end for
			
			cout << "output \"" << file_name << "\" successfully" << endl;
           	outFile.close();
        } // if
        else cout << endl << "### " << file_name << " cannot be created! ###" << endl;
	} // end pass2()
	
}; // end class CA

int main() {
	int sic_or_xe = 0;
	string file_name;
	CA sic_assembler;
	do {
		cout << "SIC(1) or SIC/XE(2): ";
		cin >> sic_or_xe;
		if ( sic_or_xe == 1 ) g_SIC = true;
		else if ( sic_or_xe == 2 ) g_SIC = false;
		else { cout << "must be \"1\" or \"2\"!!\n"; continue; }
		cout << "Input the file name(without suffix): ";
		cin >> file_name;
	} while( ! sic_assembler.inputFile( file_name ) ) ; // read a file until the file exists
	
	sic_assembler.pass1();
	sic_assembler.pass2( file_name );
	
  	system("PAUSE");	
  	return 0;
} // end main() 
