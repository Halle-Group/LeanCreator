

// This file was automatically generated by Coco/R; don't modify it.
#if !defined(busy_COCO_PARSER_H__)
#define busy_COCO_PARSER_H__

#include <QStack>
#include <busy/busySynTree.h>


namespace busy {


class Lexer;
class Parser {
private:
	enum {
		_EOF=0,
		_T_Literals_=1,
		_T_Bang=2,
		_T_BangEq=3,
		_T_Hash=4,
		_T_2Hash=5,
		_T_Dlr=6,
		_T_Percent=7,
		_T_Amp=8,
		_T_2Amp=9,
		_T_Lpar=10,
		_T_Rpar=11,
		_T_Star=12,
		_T_Rcmt=13,
		_T_StarEq=14,
		_T_Plus=15,
		_T_PlusEq=16,
		_T_Comma=17,
		_T_Minus=18,
		_T_MinusEq=19,
		_T_Dot=20,
		_T_Slash=21,
		_T_Lcmt=22,
		_T_Colon=23,
		_T_ColonEq=24,
		_T_Semi=25,
		_T_Lt=26,
		_T_Leq=27,
		_T_Eq=28,
		_T_2Eq=29,
		_T_Gt=30,
		_T_Geq=31,
		_T_Qmark=32,
		_T_Lbrack=33,
		_T_LbrackRbrack=34,
		_T_Rbrack=35,
		_T_Hat=36,
		_T_60=37,
		_T_Lbrace=38,
		_T_2Bar=39,
		_T_Rbrace=40,
		_T_Keywords_=41,
		_T_begin=42,
		_T_class=43,
		_T_define=44,
		_T_else=45,
		_T_elsif=46,
		_T_end=47,
		_T_false=48,
		_T_if=49,
		_T_import=50,
		_T_in=51,
		_T_include=52,
		_T_let=53,
		_T_param=54,
		_T_subdir=55,
		_T_submod=56,
		_T_submodule=57,
		_T_then=58,
		_T_true=59,
		_T_type=60,
		_T_var=61,
		_T_Specials_=62,
		_T_ident=63,
		_T_string=64,
		_T_integer=65,
		_T_real=66,
		_T_path=67,
		_T_symbol=68,
		_T_Eof=69,
		_T_MaxToken_=70
	};
	int maxT;

	int errDist;
	int minErrDist;

	void SynErr(int n, const char* ctx = 0);
	void Get();
	void Expect(int n, const char* ctx = 0);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);
    void SynErr(const QString& sourcePath, int line, int col, int n, const char* ctx, const QString& = QString() );

public:
	Lexer *scanner;
	struct Error
	{
		QString sourcePath;
		QString msg;
		int row, col;
	};
	QList<Error> errors;
	
	void error(const QString& sourcePath, int row, int col, const QString& msg)
	{
		Error e;
		e.sourcePath = sourcePath;
		e.row = row;
		e.col = col;
		e.msg = msg;
		errors.append(e);
	}

	Token d_cur;
	Token d_next;
	QList<Token> d_comments;
	struct TokDummy
	{
		int kind;
	};
	TokDummy d_dummy;
	TokDummy *la;			// lookahead token
	
	int peek( quint8 la = 1 );

    void RunParser();

    
busy::SynTree d_root;
	QStack<busy::SynTree*> d_stack;
	void addTerminal() {
		busy::SynTree* n = new busy::SynTree( d_cur ); d_stack.top()->d_children.append(n);
	}



	Parser(Lexer *scanner);
	~Parser();
	void SemErr(const char* msg);

	void Busy();
	void Submodule();
	void declaration();
	void statement();
	void macrodef();
	void typeref();
	void designator();
	void identdef();
	void paramList();
	void paramValue();
	void expression();
	void vardecl();
	void typedecl();
	void body();
	void initializer();
	void enumdecl();
	void classdecl();
	void fielddecl();
	void ExpList();
	void SimpleExpression();
	void relation();
	void term();
	void AddOperator();
	void factor();
	void MulOperator();
	void list();
	void constructor();
	void block();
	void condition();
	void assigOrCall();
	void assignment();
	void call();

	void Parse();

}; // end Parser

} // namespace


#endif

