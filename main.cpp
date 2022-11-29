// Calculator Parser by James Barnfather.
// 
// This is a calculator that accepts integer, decminal, and hexadecimal values
// that your can use to do basic arithmetic.
// This calculator supports + - / * ^ and executes them in the correct order.
//
// This project works by reading through your input and then converting them into tokens,
// which is then put into the parser, which uses a recursive algorithm to build an abstract sytax tree,
// which then can be execute and the result it outputted to the console;

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <cmath>
#include <bitset>
#include <sstream>

enum class Token_type
{
	none = 0,

	number,

	plus,
	minus,
	multiply,
	divide,
	power,

	left_parenthesis,
	right_parenthesis
};

struct Token
{
	Token_type type;
	double value;
};

bool is_number(const char c)
{
	return c >= '0' && c <= '9';
}

bool is_hex(const char c)
{
	return is_number(c) || ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

struct Lexer
{
	std::vector<Token> tokens;
	std::string expression;

	int loc = 0, start = 0;
	char current;

	char peek()
	{
		if (loc + 1 < expression.size())
		{
			return expression.at(loc + 1);
		}

		return '\0';
	}

	bool peek_and_advance(char c)
	{
		if (!(loc + 1 < expression.size()))
		{
			return false;
		}

		if (peek() == c)
		{
			advance();
			return true;
		}

		return false;
	}

	void advance()
	{
		if (loc + 1 < expression.size())
		{
			current = expression.at(++loc);
		}
		else
		{
			std::cout << "Advance: Out of Bounds Error.\n";
		}
	}

	Lexer(std::string e)
		: expression(e)
	{
		for(;loc < expression.size();loc++)
		{
			current = expression.at(loc);
			
			start = loc;

			Token t;
			switch (current)
			{
				case '(': t.type = Token_type::left_parenthesis; break;
				case ')': t.type = Token_type::right_parenthesis; break;

				case '+': t.type = Token_type::plus; break;
				case '-': t.type = Token_type::minus; break;
				case '*': t.type = Token_type::multiply; break;
				case '/': t.type = Token_type::divide; break;
				case '^': t.type = Token_type::power; break;

				default:
				{
					if (current == ' ' ||
						current == '/t' ||
						current == '/n')
					{
						continue;
					}

					// FIX: Crash if your type 0x with no number after.
					if (current == '0' && peek_and_advance('x'))
					{
						if (is_hex(peek()))
						{
							advance();

							start = loc;
							t.type = Token_type::number;

							while (loc + 1 < expression.size() && (is_hex(peek()) || peek() == '.'))
							{
								advance();
							}

							int decnum;
							std::stringstream(expression.substr(start, loc - start + 1)) >> std::hex >> decnum;
							t.value = static_cast<double>(decnum);
						}
						else
						{
							std::cout << "Error: Hexadecimal must have a number after it.\n";
							return;
						}
					}
					else if (is_number(current))
					{
						t.type = Token_type::number;

						while (loc + 1 < expression.size() && (is_number(peek()) || peek() == '.'))
						{
							advance();
						}

						t.value = atof(expression.substr(start, loc - start + 1).c_str());
					}
				} break;
			}

			tokens.push_back(t);
		}
	}
};

struct Node
{
	virtual double execute() { return 0.0f; };
};

struct BinaryNode: Node
{
	Node* left, *right;
	Token_type op;

	BinaryNode(Node* l, Token_type o, Node* r)
		: left(l), right(r), op(o)
	{}

	double execute() override
	{
		switch(op)
		{
			case Token_type::plus: return left->execute() + right->execute(); break;
			case Token_type::minus: return left->execute() - right->execute(); break;
			case Token_type::multiply: return left->execute() * right->execute(); break;
			case Token_type::divide: return left->execute() / right->execute(); break;
			case Token_type::power: return std::pow(left->execute(), right->execute()); break;
		}
	}
};

struct ValueNode : Node
{
	double value;

	ValueNode(double v) : value(v) {}

	double execute() override
	{
		return value;
	}
};

struct Parser
{
	Lexer lexer;
	int token_loc = 0;
	Token current_token;

	Node* tree;

	bool error = false;

	Parser(Lexer l)
		: lexer(l)
	{
		current_token = lexer.tokens.at(0);

		tree = expression(value(), 0);
		if(error)
		{
			std::cout << "Tree failed to parse the following: " << lexer.expression << '\n';
			error = true;
		}
	}

	Token peek_next_token()
	{
		if(token_loc < lexer.tokens.size() - 1)
		{
			return lexer.tokens.at(token_loc + 1);
		}

		return Token(Token_type::none);
	}

	void advance_token()
	{
		if(token_loc < lexer.tokens.size() - 1)
		{
			current_token = lexer.tokens.at(++token_loc);
		}
	}

	std::map<Token_type,int> operator_precedence =
	{
		{ Token_type::plus,0 },
		{ Token_type::minus,1 },
		{ Token_type::multiply,2 },
		{ Token_type::divide,3 },
		{ Token_type::power,4 }
	};

	int get_precendene(Token t)
	{
		if(t.type < Token_type::plus || t.type > Token_type::power)
		{
			return -1;
		}

		return operator_precedence.at(t.type);
	}

	// Parser based of of the Precedence climbing method by Martin Richards and Colin Whitby-Strevens as noted on
	// https://en.wikipedia.org/wiki/Operator-precedence_parser#Precedence_climbing_method
	Node* expression(Node* lhs, int min_precedence)
	{
		auto current = current_token;
		while(get_precendene(current) >= min_precedence)
		{
			auto op = current;
			advance_token();
			auto rhs = value();
			current = current_token;
			while(get_precendene(current) > get_precendene(op))
			{
				rhs = expression(rhs, get_precendene(op) + (get_precendene(current) > get_precendene(op) ? 1 : 0));
				current = current_token;
			}
			lhs = new BinaryNode(lhs, op.type, rhs);
		}

		return lhs;
	}

	Node* value()
	{
		auto t = current_token;
		if(t.type == Token_type::number)
		{
			advance_token();
			return new ValueNode(t.value);
		}
		else if(t.type == Token_type::minus)
		{
			t = peek_next_token();
			if(t.type == Token_type::number)
			{
				advance_token();
				advance_token();
				return new ValueNode(-t.value);
			}
		}
		else if(t.type == Token_type::left_parenthesis)
		{
			advance_token();
			Node* n = expression(value(), 0);
			if(current_token.type == Token_type::right_parenthesis)
			{
				advance_token();
				return n;
			}
		}

		error = true;
		std::cout << "Incorrect use of grammar" << "\n";

		return NULL;
	}

	double run()
	{
		if (!error)
		{
			return tree->execute();
		}
	}
};


void test(const std::string expression, const double expected)
{
	Lexer lexer(expression);
	Parser parser(lexer);

	if(parser.error)
	{
		return;
	}

	auto answer = parser.run();

	bool success = (answer == expected);
	std::cout << "Running \"" << expression << " = " << expected << "\" Calculated answer: " << answer << ", " << (success ? "Passed" : "Failed") << "\n";
}

void run_tests()
{
	std::cout << "Running unit tests.\n";
	test("2 + 5", 7);
	test("8 - 3", 5);
	test("5 * 4", 20);
	test("8 / 2", 4);
	test("4 ^ 2", 16);

	test("1 + 2 * 3", 7);
	test("(1 + 2) * 3", 9);
	test("6 + 3 - 2 + 12", 19);
	test("2 * 15 + 23", 53);
	test("10 - 3 ^ 2", 1);

	test("3.5 * 3", 10.5);
	test("-53 + -24", -77);
	// NOTE: This only fails due to there being more than three 3's;
	test("10 / 3", 3.333);
	test("(-20 * 1.8) / 2", -18);
	test("-12.315 - 42", -54.315);

	std::cout << "Unit tests complete.\n\n";
}

void calculate(std::string expression)
{
	Lexer lexer(expression);
	Parser parser(lexer);

	if (parser.error)
	{
		return;
	}

	std::cout << parser.run() << '\n';
}

int main()
{
	run_tests();

	std::cout << "Please input an expression or type 'q' to quit: ";
	std::string expression;
	while (true)
	{
		std::getline(std::cin, expression);

		if (expression == "q" || expression == "Q")
		{
			break;
		}

		std::cout << "And the Answer is: ";

		calculate(expression);
	}
}
